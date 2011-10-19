clear
clear
rm -f *.aux *.log *.toc *.thm *.idx
pdflatex main.tex && evince main.pdf
