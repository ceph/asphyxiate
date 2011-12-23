import docutils.parsers.rst
import docutils.statemachine
import logging
import os
import sphinx.errors
import sys
from lxml import etree
import docutils.nodes
import sphinx.domains.c


log = logging.getLogger(__name__)

# kludge for python <2.7
if not hasattr(log, 'getChild'):

    def getChild(name):
        return logging.getLogger(log.name + '.' + name)
    log.getChild = getChild


class AsphyxiateError(sphinx.errors.SphinxError):
    category = 'Asphyxiate error'


def listify(g):
    """Decorator that gathers generator results and returns a list."""
    def _listify(*args, **kwargs):
        r = g(*args, **kwargs)
        try:
            i = iter(r)
        except TypeError:
            # not iterable, pass through is better than raising here
            return r
        else:
            return list(i)
    return _listify


def handle_function_params(node, directive):
    assert node.get('kind') in ['param'], \
        "cannot handle {node.tag} kind={node.attrib[kind]}".format(node=node)

    doc_field_types = sphinx.domains.c.CDomain.directives['function'].doc_field_types
    # i wish i could access the "typemap" directly, this lookup is fugly
    (field_type,) = [f for f in doc_field_types if f.name == 'parameter']

    def get_items():
        for item in node.xpath("./parameteritem"):
            # TODO more than 1 entry? why would it happen?
            name = item.xpath("./parameternamelist/parametername/text()")
            (name,) = name
            content = docutils.nodes.container()
            for desc in item.xpath("./parameterdescription/*"):
                for n in render(desc, directive):
                    content.append(n)
            if len(content.children) == 1:
                # just one child, pass it straight through
                (content,) = content.children

                # as a special case, if it's a paragraph with a single
                # text blob, use that; otherwise the resulting html
                # has unnecessary blank lines
                if (content.tagname == 'paragraph'
                    and len(content.children) == 1
                    and content.children[0].tagname == '#text'):
                    content = content.children[0]

            yield (name, content)

    items = list(get_items())
    f = field_type.make_field(
        # TODO when is this needed
        types={},
        domain=directive.domain,
        items=items,
        )
    return f


def handle_function_returnval(node, directive):
    assert node.get('kind') in ['return'], \
        "cannot handle {node.tag} kind={node.attrib[kind]}".format(node=node)

    doc_field_types = sphinx.domains.c.CDomain.directives['function'].doc_field_types
    # i wish i could access the "typemap" directly, this lookup is fugly
    (field_type,) = [f for f in doc_field_types if f.name == 'returnvalue']

    content = docutils.nodes.container()
    for desc in node.xpath("./*"):
        for n in render(desc, directive):
            content.append(n)
    if len(content.children) == 1:
        # just one child, pass it straight through
        (content,) = content.children

    f = field_type.make_field(
        # TODO when is this needed
        types={},
        domain=directive.domain,
        item=(None, content),
        )
    return f


def _render_memberdef_function(node, directive):
    # TODO skip if @prot != 'public' ?
    assert node.get('prot') in ['public'], \
        "cannot handle {node.tag} kind={node.attrib[kind]}".format(node=node)

    # TODO render @static @const @explicit @inline @virt

    usage = '{type} {name}{argsstring}'.format(
        type=node.xpath("./type/text()")[0],
        name=node.xpath("./name/text()")[0],
        argsstring=node.xpath("./argsstring/text()")[0],
        )
    directive = sphinx.domains.c.CDomain.directives['function'](
        name='c:function',
        arguments=[usage],
        options={},
        # sphinx is annoying and assumes content is always a
        # StringList, never just a list
        content=docutils.statemachine.StringList([]),
        lineno=directive.lineno,
        content_offset=directive.content_offset,
        block_text='',
        state=directive.state,
        state_machine=directive.state_machine,
        )
    items = list(directive.run())
    assert items[-1].tagname == 'desc'
    assert items[-1].children[-1].tagname == 'desc_content'
    for para in node.xpath("./briefdescription/*"):
        for p in render(para, directive):
            items[-1].children[-1].append(p)

    # we need to combine parameters and returns from separate subtrees
    # under a single element, so collect them and remove them from the
    # doxygen xml (to avoid rendering twice)
    parameterlist = node.xpath("./detaileddescription//parameterlist[@kind='param']")
    for n in parameterlist:
        n.getparent().remove(n)

    returnval = node.xpath("./detaileddescription//simplesect[@kind='return']")
    for n in returnval:
        n.getparent().remove(n)

    for para in node.xpath("./detaileddescription/*"):
        for p in render(para, directive):
            items[-1].children[-1].append(p)

    l = docutils.nodes.field_list()
    for n in parameterlist:
        l.append(handle_function_params(n, directive))
    for n in returnval:
        l.append(handle_function_returnval(n, directive))
    if l.children:
        items[-1].children[-1].append(l)

    return items


def _render_memberdef_define(node, directive):
    # TODO skip if @prot != 'public' ?
    assert node.get('prot') in ['public'], \
        "cannot handle {node.tag} kind={node.attrib[kind]}".format(node=node)

    # TODO render @static

    # TODO macros and not just defines
    usage = '{name}'.format(
        name=node.xpath("./name/text()")[0],
        )
    directive = sphinx.domains.c.CDomain.directives['macro'](
        name='c:macro',
        arguments=[usage],
        options={},
        # sphinx is annoying and assumes content is always a
        # StringList, never just a list
        content=docutils.statemachine.StringList([]),
        lineno=directive.lineno,
        content_offset=directive.content_offset,
        block_text='',
        state=directive.state,
        state_machine=directive.state_machine,
        )
    items = list(directive.run())
    assert items[-1].tagname == 'desc'
    assert items[-1].children[-1].tagname == 'desc_content'
    for para in node.xpath("./briefdescription/*"):
        for p in render(para, directive):
            items[-1].children[-1].append(p)
    for para in node.xpath("./detaileddescription/*"):
        for p in render(para, directive):
            items[-1].children[-1].append(p)

    return items


def _render_memberdef_typedef(node, directive):
    # TODO skip if @prot != 'public' ?
    assert node.get('prot') in ['public'], \
        "cannot handle {node.tag} kind={node.attrib[kind]}".format(node=node)

    # TODO render @static

    usage = '{name}'.format(
        name=node.xpath("./name/text()")[0],
        )
    directive = sphinx.domains.c.CDomain.directives['type'](
        name='c:type',
        arguments=[usage],
        options={},
        # sphinx is annoying and assumes content is always a
        # StringList, never just a list
        content=docutils.statemachine.StringList([]),
        lineno=directive.lineno,
        content_offset=directive.content_offset,
        block_text='',
        state=directive.state,
        state_machine=directive.state_machine,
        )
    items = list(directive.run())
    assert items[-1].tagname == 'desc'
    assert items[-1].children[-1].tagname == 'desc_content'
    for para in node.xpath("./briefdescription/*"):
        for p in render(para, directive):
            items[-1].children[-1].append(p)
    for para in node.xpath("./detaileddescription/*"):
        for p in render(para, directive):
            items[-1].children[-1].append(p)

    return items


def _render_memberdef_variable(node, directive):
    # TODO this is really about struct members, currently
    # TODO skip if @prot != 'public' ?
    assert node.get('prot') in ['public'], \
        "cannot handle {node.tag} kind={node.attrib[kind]}".format(node=node)

    # TODO render @static @mutable
    # TODO when do variables have @argsstring
    # TODO what is @inbodydescription

    usage = '{type} {name}'.format(
        type=node.xpath("./type/text()")[0],
        name=node.xpath("./name/text()")[0],
        )
    directive = sphinx.domains.c.CDomain.directives['member'](
        name='c:member',
        arguments=[usage],
        options={},
        # sphinx is annoying and assumes content is always a
        # StringList, never just a list
        content=docutils.statemachine.StringList([]),
        lineno=directive.lineno,
        content_offset=directive.content_offset,
        block_text='',
        state=directive.state,
        state_machine=directive.state_machine,
        )
    items = list(directive.run())
    assert items[-1].tagname == 'desc'
    assert items[-1].children[-1].tagname == 'desc_content'
    for para in node.xpath("./briefdescription/*"):
        for p in render(para, directive):
            items[-1].children[-1].append(p)
    for para in node.xpath("./detaileddescription/*"):
        for p in render(para, directive):
            items[-1].children[-1].append(p)

    return items


def render_memberdef(node, directive):
    kind = node.get('kind')
    fn = globals().get('_render_{name}_{kind}'.format(
            name=node.tag,
            kind=kind,
            ))
    assert fn is not None, \
        "cannot handle {node.tag} kind={node.attrib[kind]}".format(node=node)

    return fn(node, directive)


def render_sectiondef(node, directive):
    TITLES = {
        'func': 'Functions',
        'define': 'Defines',
        'typedef': 'Types',
        # TODO this is really about struct members, currently
        'public-attrib': 'Members',
        }
    kind = node.get('kind')
    title = TITLES.get(kind)
    assert title is not None, \
        "cannot handle {node.tag} kind={node.attrib[kind]}".format(node=node)

    sec = docutils.nodes.section(ids=[kind])
    sec.append(docutils.nodes.title(text=title))

    for child in node.xpath("./*[not(self::location)]"):
        for item in render(child, directive):
            sec.append(item)
    return [sec]


def _render_compounddef_file(node, directive):
    # TODO render compoundname, briefdescription, detaileddescription
    # listofallmembers seems to just duplicate the sectiondef>memberdef's
    for child in node:
        if child.tag in [
            'compoundname',
            'briefdescription',
            'detaileddescription',
            'location',
            'listofallmembers',
            ]:
            continue
        for item in render(child, directive):
            yield item


def _render_compounddef_struct(node, directive):
    # TODO skip if @prot != 'public' ?
    title = 'Struct {name}'.format(
        name=node.xpath("./compoundname/text()")[0],
        )
    sec = docutils.nodes.section(ids=[title])
    sec.append(docutils.nodes.title(text=title))

    usage = 'struct {name}'.format(
        name=node.xpath("./compoundname/text()")[0],
        )
    directive = sphinx.domains.c.CDomain.directives['type'](
        name='c:type',
        arguments=[usage],
        options={},
        # sphinx is annoying and assumes content is always a
        # StringList, never just a list
        content=docutils.statemachine.StringList([]),
        lineno=directive.lineno,
        content_offset=directive.content_offset,
        block_text='',
        state=directive.state,
        state_machine=directive.state_machine,
        )
    items = list(directive.run())
    assert items[-1].tagname == 'desc'
    assert items[-1].children[-1].tagname == 'desc_content'
    for para in node.xpath("./briefdescription/*"):
        for p in render(para, directive):
            items[-1].children[-1].append(p)
    for para in node.xpath("./detaileddescription/*"):
        for p in render(para, directive):
            items[-1].children[-1].append(p)

    sec.extend(items)

    for child in node:
        if child.tag in [
            'compoundname',
            # not sure what use this would be
            'includes',
            'briefdescription',
            'detaileddescription',
            'location',
            # listofallmembers seems to just duplicate the
            # sectiondef>memberdef's
            'listofallmembers',
            ]:
            continue
        for item in render(child, directive):
            sec.append(item)
    return [sec]


def render_compounddef(node, directive):
    kind = node.get('kind')
    fn = globals().get('_render_{name}_{kind}'.format(
            name=node.tag,
            kind=kind,
            ))
    assert fn is not None, \
        "cannot handle {node.tag} kind={node.attrib[kind]}".format(node=node)

    return fn(node, directive)


def render_compound(node, directive):
    assert node.get('kind') in ['file'], \
        "cannot handle {node.tag} kind={node.attrib[kind]}".format(node=node)

    refid = node.attrib['refid']
    env = directive.state.document.settings.env
    xml_path = env.config.asphyxiate_doxygen_xml
    path = os.path.join(
        xml_path,
        'xml',
        '{refid}.xml'.format(refid=refid),
        )
    log.getChild('render_compound').debug('Parsing doxygen xml from %s', path)
    xml = etree.parse(path)
    for node in xml.getroot():
        for item in render(node, directive):
            yield item


def render_innerclass(node, directive):
    # TODO skip if @prot != 'public' ?
    refid = node.attrib['refid']
    env = directive.state.document.settings.env
    xml_path = env.config.asphyxiate_doxygen_xml
    path = os.path.join(
        xml_path,
        'xml',
        '{refid}.xml'.format(refid=refid),
        )
    log.getChild('render_innerclass').debug(
        'Parsing doxygen xml from %s', path,
        )
    xml = etree.parse(path)
    for node in xml.getroot():
        for item in render(node, directive):
            yield item


def render_para(node, directive):
    p = docutils.nodes.paragraph()
    if node.text is not None:
        # TODO this isn't always safe.. how do i know when it's safe?
        # basically, <para>foo <itemizedlist>... should eat the
        # whitespace, <para>foo <em>blah</em> should not? does doxygen
        # even have inline markup? try to go with the heuristic of
        # eating whitespace before a closing tag, defer the rest until
        # it's actually a problem.
        text = node.text
        if len(node) == 0:
            text = text.strip()
        p.append(docutils.nodes.Text(text))
    for child in node:
        for item in render(child, directive):
            p.append(item)
        if child.tail is not None:
            tail = child.tail.strip()
            if tail:
                p.append(docutils.nodes.Text(tail))
    return [p]


def render_itemizedlist(node, directive):
    l = docutils.nodes.bullet_list()
    assert node.text is None or node.text.strip() == ''
    for child in node:
        for item in render(child, directive):
            l.append(item)
        assert child.tail is None
    return [l]


def render_listitem(node, directive):
    i = docutils.nodes.list_item()
    assert node.text is None or node.text.strip() == ''
    for child in node:
        for item in render(child, directive):
            i.append(item)
        assert child.tail is None
    return [i]


def render_ref(node, directive):
    ROLES = dict(
        member='func',
        compound='data',
        )
    role = ROLES.get(node.get('kindref'))
    assert role is not None, \
        "cannot handle {node.tag} kind={node.attrib[kindref]}".format(node=node)

    usage = node.xpath("./text()")[0]
    items, _ = sphinx.domains.c.CDomain.roles[role](
        typ='{name}:{role}'.format(name=sphinx.domains.c.CDomain.name, role=role),
        rawtext='',
        text=usage,
        lineno=directive.lineno,
        inliner=directive.state_machine,
        )
    return items


def _render_simplesect_warning(node, directive):
    w = docutils.nodes.warning()
    for n in node:
        w.extend(render(n, directive))
    return [w]


def _render_simplesect_note(node, directive):
    note = docutils.nodes.note()
    for n in node:
        note.extend(render(n, directive))
    return [note]


def _render_simplesect_pre(node, directive):
    p = docutils.nodes.admonition()
    p['classes'].append('admonition-precondition')
    p.append(docutils.nodes.title(text='Precondition'))
    for n in node:
        p.extend(render(n, directive))
    return [p]


def _render_simplesect_post(node, directive):
    p = docutils.nodes.admonition()
    p['classes'].append('admonition-postcondition')
    p.append(docutils.nodes.title(text='Postcondition'))
    for n in node:
        p.extend(render(n, directive))
    return [p]


def render_simplesect(node, directive):
    kind = node.get('kind')
    fn = globals().get('_render_{name}_{kind}'.format(
            name=node.tag,
            kind=kind,
            ))
    assert fn is not None, \
        "cannot handle {node.tag} kind={node.attrib[kind]}".format(node=node)

    return fn(node, directive)


def render(node, directive):
    log.getChild('render').debug('Rendering %s', node.tag)
    fn = globals().get('render_{name}'.format(name=node.tag))
    if fn is None:
        warning = 'asphyxiate: {msg} {tag!r}'.format(
            msg='No renderer found for doxygen tag',
            tag=node.tag,
            )
        yield docutils.nodes.warning(
            "",
            docutils.nodes.paragraph("", "", docutils.nodes.Text(warning)),
            docutils.nodes.literal_block(text=etree.tostring(node, pretty_print=True).rstrip()),
            )
        yield directive.state.document.reporter.warning(
            warning,
            line=directive.lineno,
            )
    else:
        for item in fn(node, directive):
            yield item


class AsphyxiateFileDirective(docutils.parsers.rst.Directive):

    required_arguments = 1

    @listify
    def run(self):
        (filename,) = self.arguments

        env = self.state.document.settings.env
        xml_path = env.config.asphyxiate_doxygen_xml
        if xml_path is None:
            raise AsphyxiateError(
                'missing config setting asphyxiate_doxygen_xml')

        path = os.path.join(xml_path, 'xml', 'index.xml')
        log.debug('Parsing xml: %s', path)
        index_xml = etree.parse(path)
        for node in index_xml.xpath(
            "//compound[@kind='file' and name=$name]",
            name=filename,
            ):
            for item in render(node, self):
                yield item


def setup(app):
    if app is sys.modules['asphyxiate']:
        # nose mistakenly thinks this is a module-level setup
        # function, but Sphinx dictates this function name.. tell nose
        # to keep its.. nose.. out of other people's business
        return

    # sphinx isn't helpful for extensions wanting to log, so bypass it and
    # go straight to stderr
    log.propagate = False
    handler = logging.StreamHandler()
    handler.setFormatter(
        logging.Formatter(fmt='%(name)s:%(levelname)s: %(message)s'),
        )
    log.addHandler(handler)
    log.setLevel(logging.DEBUG)  # TODO take this from configuration

    app.add_directive(
        "doxygenfile",
        AsphyxiateFileDirective,
        )

    app.add_config_value('asphyxiate_doxygen_xml', None, 'html')
