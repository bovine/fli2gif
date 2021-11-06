CXX := gcc++

fli2gif: fli2gif.cpp fliplay.cpp gif.cpp
	$(CXX) fli2gif.cpp fliplay.cpp gif.cpp -o fli2gif

clean:
	rm -f fli2gif
.PHONY: clean
