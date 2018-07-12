# Documentation
The following files are available in this folder:

* `tutorial.pdf` contains the Tutorials for the event receiver, tailored for PSI users.
* `tutorial.tex` contains the LaTeX source for the tutorials.
* `evr_manual.pdf` contains the EVR Manual.
* `evr_manual.tex` contains the LaTeX source for the EVR Manual.
* `references.bib` contains the bibliography for the Tutorials and the EVR Manual in BibTeX format.

Folder `img` contains images used in the Tutorial and the EVR Manual in _svg_ format. 
Folder `doxy` contains generated doxygen documentation.

## Building the documentation from source
__Prerequisites:__

* [Inkscape](https://inkscape.org/en/) tool for converting _svg_ image format to _pdf_ format.
* LaTeX environment (pdflatex, bibtex).
* Doxygen

In order to build the documentation from source, follow theese simple steps:

1. To build the tutorial and manual:

        make

2. To build the doxygen documentation run the following command in the main project folder `mrfioc2`:

		doxygen

