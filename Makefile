run: gnoetics/xxx_gnoetics.so texts-ts
	(cd interface && ./gnoetry)

clean: clean-lib clean-texts

clean-lib:
	-rm -rf gnoetics/xxx_gnoetics.so gnoetics/build
clean-texts:
	-rm -rf texts-ts

gnoetics/xxx_gnoetics.so: gnoetics/gnoetics.c
	(cd gnoetics && python setup.py build && \
	ln -s build/lib*/xxx_gnoetics.so .)

texts-ts:
	mkdir texts-ts
	(cd tools && ./tokenize-all.py)
