from setuptools import setup, find_packages

setup(
    name='asphyxiate',
    version='0.0.1',
    packages=find_packages(),

    author='Tommi Virtanen',
    author_email='tommi.virtanen@dreamhost.com',
    description='Sphinx plugin for code documentation via Doxygen',
    license='MIT',
    keywords='doxygen documentation sphinx',

    install_requires=[
        'setuptools',
        'Sphinx >=1.0.7',
        'lxml >=2.3.2',
        ],

    classifiers=[
        'Development Status :: 3 - Alpha',
        'Environment :: Plugins',
        'Intended Audience :: Developers',
        'Programming Language :: Python',
        'Topic :: Software Development :: Documentation',
        'Topic :: Text Processing :: Markup',
        'Topic :: Documentation',
        'License :: OSI Approved :: MIT License',
        ],
    )
