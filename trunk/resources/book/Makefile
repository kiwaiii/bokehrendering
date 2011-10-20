#clear
#clear
#rm -f *.aux *.log *.toc *.thm *.idx
#pdflatex main.tex && evince main.pdf
#pdflatex main && bibtex main && pdflatex main && pdflatex main  && evince main.pdf

all   : main.pdf

main.pdf :
	pdflatex main
	bibtex main
	pdflatex main
	pdflatex main

clean :
	rm -f *.aux *.log *.toc *.thm *.idx *.bbl *.blg *.pdf



