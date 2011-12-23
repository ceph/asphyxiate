======================================================================
 Asphyxiate -- Sphinx plugin to embed code documentation from Doxygen
======================================================================

Asphyxiate is a Sphinx_ plugin that pulls source code documentation
from C/C++ via Doxygen_.

.. _Sphinx: http://sphinx.pocoo.org/
.. _Doxygen: http://doxygen.org/

It is meant to handle large projects well. For example, Ceph_ is an
open source project with some 175,000 lines of C++/C code. The
Doxygen_ XML output for it is about 64MB total. A single
``doxygenfile`` directive with Breathe_ takes 2 minutes to process,
and parses every single one of the Doxygen XML files, for no apparent
reason. Digging in deeper made us realize we might be `better off`_ with
a from-scratch rewrite.

.. _Ceph: http://ceph.newdream.net/
.. _Breathe: https://github.com/michaeljones/breathe
.. _`better off`: https://github.com/michaeljones/breathe/blob/1d15060a570e498b2eb8dac3ee10cc21dc998801/breathe/renderer/rst/doxygen/filter.py#L269
