import docutils.parsers.rst
import os
import sphinx.errors
import sys
from lxml import etree
import docutils.nodes


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

        index_xml = etree.parse(os.path.join(xml_path, 'xml', 'index.xml'))
        for node in index_xml.xpath(
            "//compound[@kind='file' and name=$name]",
            name=filename,
            ):
            refid = node.get('refid')
            path = os.path.join(
                xml_path,
                'xml',
                '{refid}.xml'.format(refid=refid),
                )
            file_xml = etree.parse(path)
            yield docutils.nodes.literal_block(text=etree.tostring(node))
            yield docutils.nodes.literal_block(text=etree.tostring(file_xml))


def setup(app):
    if app is sys.modules['asphyxiate']:
        # nose mistakenly thinks this is a module-level setup
        # function, but Sphinx dictates this function name.. tell nose
        # to keep its.. nose.. out of other people's business
        return

    app.add_directive(
        "doxygenfile",
        AsphyxiateFileDirective,
        )

    app.add_config_value('asphyxiate_doxygen_xml', None, 'html')
