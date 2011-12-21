import os
import shutil

from .util import doxygen, sphinx


def test_example():
    src = os.path.join(os.path.dirname(__file__), 'example-src')
    xml = os.path.join(os.path.dirname(__file__), 'example-xml')
    if os.path.isdir(xml):
        shutil.rmtree(xml)
    os.mkdir(xml)
    doxygen(src=src, xml=xml)
    rst = os.path.join(os.path.dirname(__file__), 'example-rst')
    sphinxtmp = os.path.join(os.path.dirname(__file__), 'example-sphinxtmp')
    if os.path.isdir(sphinxtmp):
        shutil.rmtree(sphinxtmp)
    os.mkdir(sphinxtmp)
    html = os.path.join(os.path.dirname(__file__), 'example-html')
    if os.path.isdir(html):
        shutil.rmtree(html)
    os.mkdir(html)
    sphinx(rst=rst, xml=xml, tmp=sphinxtmp, html=html)
