all: doxygen install

doxygen:
	doxygen

install:
	cp -f *.html /home/epics/web_software/epics
	cp -rfp tomoReconDoxygenHTML  /home/epics/web_software/epics/

clean:
	rm -rf tomoReconDoxygenHTML/*


realclean: clean
