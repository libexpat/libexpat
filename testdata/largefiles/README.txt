This directory contains some really large test files, mostly used to
benchmark various aspects of Expat's performance.

(As files are added, they should be described here, including what
benchmark program they're intended to be used with and what that
resulting measurements tell us.)

* nes96.xml (~2.8MB): 
  - properties: no namespaces, mixed content, average nesting depth
  - source: http://sda.berkeley.edu:7502/ddi/nes96/
    (no indication of license or copyright there)
  - purpose: mostly for performance testing with the benchmark utility

* wordnet_glossary-20010201.xml (~14.4MB): 
  - properties: namespaces, element content, flat 
  - source: http://www.semanticweb.org/library/wordnet/
    (license looks Open Source, see license.html file on the same page)
  - purpose: mostly for performance testing with the benchmark utility

* recset.xml (~29.1MB): 
  - properties: small portion with namespaces, bulk without, element content, flat 
  - test data donated by Karl Waclawek
  - purpose: mostly for performance testing with the benchmark utility

