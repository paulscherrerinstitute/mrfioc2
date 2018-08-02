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

In order to build the documentation from source, follow theese simple steps:

1. if you have not already, git clone the reporsitory

		git clone https://skube_s@github.psi.ch/scm/ed/mrfioc2.git

2. To build the doxygen documentation run the following command in the main project folder `mrfioc2`:

		doxygen

3. Change current directory to the documentation folder

		cd mrfioc2/documentation

4. Convert the images to pdf using the [inkscape](https://inkscape.org/en/) tool:
 
		inkscape -C -z --file=img/CML.svg --export-pdf=img/CML.pdf 
		inkscape -C -z --file=img/dbus.svg --export-pdf=img/dbus.pdf
		inkscape -C -z --file=img/univ.svg --export-pdf=img/univ.pdf 
		inkscape -C -z --file=img/TTL.svg --export-pdf=img/TTL.pdf 
		inkscape -C -z --file=img/softEvent.svg --export-pdf=img/softEvent.pdf 
		inkscape -C -z --file=img/pulserSignal.svg --export-pdf=img/pulserSignal.pdf
		inkscape -C -z --file=img/pulserGeneric.svg --export-pdf=img/pulserGeneric.pdf
		inkscape -C -z --file=img/pulser.svg --export-pdf=img/pulser.pdf
		inkscape -C -z --file=img/prescaler.svg --export-pdf=img/prescaler.pdf
		inkscape -C -z --file=img/output.svg --export-pdf=img/output.pdf
		inkscape -C -z --file=img/templates.svg --export-pdf=img/templates.pdf
		inkscape -C -z --file=img/gen_evt_clk.svg --export-pdf=img/gen_evt_clk.pdf

5. Build the Tutorials

        pdflatex -synctex=1 -interaction=nonstopmode "tutorial".tex
        bibtex "tutorial".aux
        pdflatex -synctex=1 -interaction=nonstopmode "tutorial".tex
        pdflatex -synctex=1 -interaction=nonstopmode "tutorial".tex

6. Build the EVR Manual

		pdflatex -synctex=1 -interaction=nonstopmode "evr_manual".tex
		bibtex "evr_manual".aux
		pdflatex -synctex=1 -interaction=nonstopmode "evr_manual".tex
		pdflatex -synctex=1 -interaction=nonstopmode "evr_manual".tex
