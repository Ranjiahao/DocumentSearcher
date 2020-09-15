FLAG=-std=c++11 -I ~/third_part/include

index_test:searcher.cc index_test.cc
	g++ $^ -o $@ $(FLAG)

.PHONY:clean
clean:
	rm index_test
