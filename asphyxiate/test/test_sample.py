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
    assert got[0].tag == 'h1'
    del got[0]
    assert want[0].tag == 'h1'
    del want[0]
    (got,) = got
    (want,) = want
    for tree in [got, want]:
        for node in tree.xpath('//*[@id]'):
            del node.attrib['id']
        for node in tree.xpath('//*[@href]'):
            del node.attrib['href']

    got = lxml.html.tostring(got, pretty_print=True)
    want = lxml.html.tostring(want, pretty_print=True)

    # they can't have the same identifiers, so "want" side puts XYZZY
    # in every identifier
    want = want.replace('XYZZY', '')

    eq(got, want)


def test_sample():
    path = os.path.join(os.path.dirname(__file__), 'sample')
    samples = sorted(
        fn for fn in os.listdir(path)
        if not fn.startswith('.')
        )
    for name in samples:
        def _test():
            return _test_sample(name, os.path.join(path, name))
        _test.description = 'test_sample({name!r})'.format(name=name)
        yield _test
