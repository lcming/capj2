PROJECT=pj2
TEX=pdflatex
READER=evince
BIBTEX=bibtex

OPEN: BUILDTEX
	$(READER) $(PROJECT).pdf

BUILDTEX:
	$(TEX) $(PROJECT).tex
	$(BIBTEX) $(PROJECT)
	$(TEX) $(PROJECT).tex
	$(TEX) $(PROJECT).tex
    
clean:
	rm -rf $(PROJECT).aux $(PROJECT).bbl  $(PROJECT).blg *.log $(PROJECT).pdf $(PROJECT).ps $(PROJECT).out


