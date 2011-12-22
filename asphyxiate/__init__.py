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
    contents = []
    for s in node.xpath("./briefdescription/para/text()"):
        contents.append(s)
        contents.append('')
    for s in node.xpath("./detaileddescription/para/text()"):
        contents.append(s)
        contents.append('')
    directive = sphinx.domains.c.CDomain.directives['function'](
        name='c:function',
        arguments=[usage],
        options={},
        # sphinx is annoying and assumes content is always a
        # StringList, never just a list
        content=docutils.statemachine.StringList(contents),
        lineno=directive.lineno,
        content_offset=directive.content_offset,
        block_text='',
        state=directive.state,
        state_machine=directive.state_machine,
        )
    for item in directive.run():
        yield item


def _render_memberdef_define(node, directive):
    # TODO skip if @prot != 'public' ?
    assert node.get('prot') in ['public'], \
        "cannot handle {node.tag} kind={node.attrib[kind]}".format(node=node)

    # TODO render @static

    # TODO macros and not just defines
    usage = '{name}'.format(
        name=node.xpath("./name/text()")[0],
        )
    contents = []
    for s in node.xpath("./briefdescription/para/text()"):
        contents.append(s)
        contents.append('')
    for s in node.xpath("./detaileddescription/para/text()"):
        contents.append(s)
        contents.append('')
    directive = sphinx.domains.c.CDomain.directives['macro'](
        name='c:macro',
        arguments=[usage],
        options={},
        # sphinx is annoying and assumes content is always a
        # StringList, never just a list
        content=docutils.statemachine.StringList(contents),
        lineno=directive.lineno,
        content_offset=directive.content_offset,
        block_text='',
        state=directive.state,
        state_machine=directive.state_machine,
        )
    for item in directive.run():
        yield item


def _render_memberdef_typedef(node, directive):
    # TODO skip if @prot != 'public' ?
    assert node.get('prot') in ['public'], \
        "cannot handle {node.tag} kind={node.attrib[kind]}".format(node=node)

    # TODO render @static

    usage = '{name}'.format(
        name=node.xpath("./name/text()")[0],
        )
    contents = []
    for s in node.xpath("./briefdescription/para/text()"):
        contents.append(s)
        contents.append('')
    for s in node.xpath("./detaileddescription/para/text()"):
        contents.append(s)
        contents.append('')
    directive = sphinx.domains.c.CDomain.directives['type'](
        name='c:type',
        arguments=[usage],
        options={},
        # sphinx is annoying and assumes content is always a
        # StringList, never just a list
        content=docutils.statemachine.StringList(contents),
        lineno=directive.lineno,
        content_offset=directive.content_offset,
        block_text='',
        state=directive.state,
        state_machine=directive.state_machine,
        )
    for item in directive.run():
        yield item


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
    contents = []
    for s in node.xpath("./briefdescription/para/text()"):
        contents.append(s)
        contents.append('')
    for s in node.xpath("./detaileddescription/para/text()"):
        contents.append(s)
        contents.append('')
    directive = sphinx.domains.c.CDomain.directives['member'](
        name='c:member',
        arguments=[usage],
        options={},
        # sphinx is annoying and assumes content is always a
        # StringList, never just a list
        content=docutils.statemachine.StringList(contents),
        lineno=directive.lineno,
        content_offset=directive.content_offset,
        block_text='',
        state=directive.state,
        state_machine=directive.state_machine,
        )
    for item in directive.run():
        yield item


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

    sec = docutils.nodes.section(ids=kind)
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
    sec = docutils.nodes.section(ids=title)
    sec.append(docutils.nodes.title(text=title))

    usage = 'struct {name}'.format(
        name=node.xpath("./compoundname/text()")[0],
        )
    contents = []
    for s in node.xpath("./briefdescription/para/text()"):
        contents.append(s)
        contents.append('')
    for s in node.xpath("./detaileddescription/para/text()"):
        contents.append(s)
        contents.append('')
    directive = sphinx.domains.c.CDomain.directives['type'](
        name='c:type',
        arguments=[usage],
        options={},
        # sphinx is annoying and assumes content is always a
        # StringList, never just a list
        content=docutils.statemachine.StringList(contents),
        lineno=directive.lineno,
        content_offset=directive.content_offset,
        block_text='',
        state=directive.state,
        state_machine=directive.state_machine,
        )
    for item in directive.run():
        sec.append(item)

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
