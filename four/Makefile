all: site-tester getsites

site-tester: 
	g++ -std=c++11 site-tester.cpp -o site-tester

getsites:
	g++ -std=c++11 -lcurl getsites.cpp -o getsites

clean:
	rm -f site-tester getsites