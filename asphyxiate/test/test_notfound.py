import os
import shutil
import lxml.html

from nose.tools import eq_ as eq

from .util import doxygen, sphinx


def test_notfound():
    path = os.path.join(os.path.dirname(__file__), 'notfound')
    if os.path.isdir(path):
        shutil.rmtree(path)
    os.mkdir(path)
    src = os.path.join(path, 'src')
    os.mkdir(src)
    with file(os.path.join(src, 'red-herring.c'), 'w') as f:
        f.write('int a;\n')
    xml = os.path.join(path, 'xml')
    os.mkdir(xml)
    doxygen(src=src, xml=xml)
    rst = os.path.join(path, 'rst')
    os.mkdir(rst)
    with file(os.path.join(rst, 'contents.rst'), 'w') as f:
        f.write('.. doxygenfile:: not-found.c\n')
    sphinxtmp = os.path.join(path, 'sphinxtmp')
    os.mkdir(sphinxtmp)
    html = os.path.join(path, 'html')
    os.mkdir(html)
    sphinx(rst=rst, xml=xml, tmp=sphinxtmp, html=html)
    # TODO check that it fails
