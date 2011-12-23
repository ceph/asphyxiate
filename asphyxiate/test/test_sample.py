import os
import shutil
import lxml.html

from nose.tools import eq_ as eq

from .util import doxygen, sphinx


def _test_sample(name, path):
    src = os.path.join(path, 'src')
    xml = os.path.join(path, 'xml')
    if os.path.isdir(xml):
        shutil.rmtree(xml)
    os.mkdir(xml)
    doxygen(src=src, xml=xml)
    rst = os.path.join(path, 'rst')
    sphinxtmp = os.path.join(path, 'sphinxtmp')
    if os.path.isdir(sphinxtmp):
        shutil.rmtree(sphinxtmp)
    os.mkdir(sphinxtmp)
    html = os.path.join(path, 'html')
    if os.path.isdir(html):
        shutil.rmtree(html)
    os.mkdir(html)
    sphinx(rst=rst, xml=xml, tmp=sphinxtmp, html=html)

    doc = lxml.html.parse(os.path.join(html, 'contents.html'))
    got = doc.xpath("id('got')/*")
    want = doc.xpath("id('want')/*")
    have_want = bool(want)

    assert got[0].tag == 'h1'
    del got[0]
    if have_want:
        assert want[0].tag == 'h1'
        del want[0]
    for treelist in [got, want]:
        for tree in treelist:
            for node in tree.xpath('//*[@id]'):
                del node.attrib['id']
            for node in tree.xpath('//*[@href]'):
                del node.attrib['href']

    # for some reason, i get 8*'<span></span>' when i generate the
    # docutils nodes in render_sectionddef
    for tree in got:
        for node in tree.xpath("//span[not(node())]"):
            node.getparent().remove(node)

    # annoying whitespace differences, not relevant most of the time
    for tree in got:
        for node in tree.xpath("//p[not(node())]"):
            node.getparent().remove(node)

    got = '\n'.join(lxml.html.tostring(tree, pretty_print=True)
                    for tree in got)
    want = '\n'.join(lxml.html.tostring(tree, pretty_print=True)
                     for tree in want)

    # more annoying whitespace differences, not relevant most of the time
    got = got.replace('\n\n', '\n')
    want = want.replace('\n\n', '\n')

    # they can't have the same identifiers, so "want" side puts XYZZY
    # in every identifier
    want = want.replace('XYZZY', '')

    if have_want:
        eq(got, want)


def test_sample():
    path = os.path.join(os.path.dirname(__file__), 'sample')
    samples = sorted(fn for fn in os.listdir(path) if not fn.startswith('.'))
    for name in samples:
        def _test():
            return _test_sample(name, os.path.join(path, name))
        _test.description = 'test_sample({name!r})'.format(name=name)
        yield _test
