PDFLATEX=pdflatex -interaction=nonstopmode
BIBTEX=bibtex

SVGS=$(shell find img/ -name *.svg)
DOCS=$(shell find . -maxdepth 1 -name '*.tex')
BIBS=$(shell find . -maxdepth 1 -name '*.bib')

all: $(DOCS:%.tex=%.pdf)
 
$(DOCS:%.tex=%.pdf): %.pdf: %.tex $(SVGS:%.svg=%.pdf) $(BIBS)
	-$(PDFLATEX) $<
	-$(BIBTEX) $(patsubst %.pdf,%.aux, $@)
	-$(PDFLATEX) $<
	-$(PDFLATEX) $<

$(SVGS:%.svg=%.pdf): %.pdf: %.svg
	inkscape -C -z --file=$^ --export-pdf=$@

clean: 
	rm -rf *.aux *.bbl *.log *.out *.blg *.toc

cleaner: clean
	rm -rf img/*.pdf *.pdf
