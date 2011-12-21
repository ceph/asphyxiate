import os
import subprocess
import sys


def doxygen(src, xml):
    conf = os.path.join(xml, 'doxygen.conf')
    with file(conf, 'w') as f:
        f.write("""
OUTPUT_DIRECTORY = {xml}
STRIP_FROM_PATH = {src}/
STRIP_FROM_INC_PATH = {src}/
BUILTIN_STL_SUPPORT = YES
WARN_IF_UNDOCUMENTED = NO
INPUT = {src}
EXAMPLE_PATH = {src}
RECURSIVE = YES
VERBATIM_HEADERS = NO
GENERATE_HTML = NO
GENERATE_LATEX = NO
GENERATE_XML = YES
XML_PROGRAMLISTING = NO
JAVADOC_AUTOBRIEF = YES
""".format(
        src=src,
        xml=xml,
        ))
    p = subprocess.Popen(
        args=[
            'doxygen',
            conf,
            ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        )
    (out, err) = p.communicate()
    if out:
        print '\n'.join('doxygen: ' + l for l in out.splitlines())
    if err:
        raise RuntimeError(
            'Doxygen gave warnings:\n'
            + '\n'.join('  ' + l for l in err.splitlines()),
            )
    if p.returncode != 0:
        raise RuntimeError('Doxygen failed: %r' % p.returncode)


def sphinx(rst, xml, tmp, html):
    with file(os.path.join(tmp, 'conf.py'), 'w') as f:
        f.write("""
extensions = ['asphyxiate']
asphyxiate_doxygen_xml = {xml!r}
""".format(
                xml=xml,
                ))

    env = {}
    env.update(os.environ)
    env['PATH'] = os.path.dirname(sys.executable) + ':' + env['PATH']
    p = subprocess.Popen(
        args=[
            'sphinx-build',
            '-a',
            '-b', 'html',
            '-c', tmp,
            rst,
            html,
            ],
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        )
    (out, err) = p.communicate()
    if out:
        print '\n'.join('sphinx: ' + l for l in out.splitlines())
    if err:
        raise RuntimeError(
            'Sphinx gave warnings:\n'
            + '\n'.join('  ' + l for l in err.splitlines()),
            )
    if p.returncode != 0:
        raise RuntimeError('Sphinx failed: %r' % p.returncode)
