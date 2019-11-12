
BDIR:=./build

VPATH := src:.

GTEST_INCLUDEDIR := $(shell if [[ -d /usr/include/gtest ]]; then echo /usr/include ; fi )
GTEST_LIBDIR := $(shell if [[ -f /usr/lib64/libgtest.so ]]; then echo /usr/lib64 ; fi )

TARGETS:= $(BDIR)/shared_q_test $(BDIR)/q_bandwidth

ifneq ($(GTEST_INCLUDEDIR),)
	TARGETS += $(BDIR)/gtest_mpmc
endif

ISBOOSTAVAILABLE:=$(shell if [[ -f /usr/include/boost/lockfree/queue.hpp ]] ; then echo 1 ; else echo 0 ; fi)

#CXX:=g++
CXX:=clang++
#CXXFLAGS:= --std=c++17 -mcx16 -pthread -W -Wall -Wshadow -Wextra -Wpedantic -I. -Isrc -O2 -ggdb3 -lpthread
CXXFLAGS:= --std=c++17 -mcx16 -pthread -W -Wall -Wshadow -Wextra -Wpedantic -I. -Isrc -O3 -march=native -mtune=native -flto
LDFLAGS := -lpthread
LINK.o := $(LINK.cc)

.PHONY: all
all: $(TARGETS)


$(BDIR)/gtest_mpmc: $(BDIR)/gtest_mpmc_queue.o
	$(CXX) $(CXXFLAGS) -I. -Isrc -DGTEST_HAS_PTHREAD=1 -pthread $< -l gtest -latomic -o $@

$(BDIR)/gtest_mpmc_queue.o: gtest_mpmc_queue.cpp mpmc_queue.h | $(BDIR)/.f
	$(CXX) $(CXXFLAGS) -I. -Isrc -DGTEST_HAS_PTHREAD=1 -pthread $< -c -o $@

$(BDIR)/shared_q_test: $(BDIR)/shared_q_test.o
$(BDIR)/shared_q_test.o : shared_q_test.cpp mpmc_queue.h | $(BDIR)/.f

.PHONY: gtest_run
gtest_run: $(BDIR)/gtest_mpmc
	$(BDIR)/gtest_mpmc

$(BDIR)/q_bandwidth.o: q_bandwidth.cpp mpmc_queue.h mpmc_queue_timing.h | $(BDIR)/.f
	$(CXX) $(CXXFLAGS) -I. -Isrc -DCOMPARE_BOOST=$(ISBOOSTAVAILABLE) -pthread $< -c -o $@

$(BDIR)/q_bandwidth: $(BDIR)/q_bandwidth.o

.PHONY: report
report: | $(BDIR)/q_bandwidth
	./scripts/bw-report.sh ./build/q_bandwidth

$(BDIR)/%.o: $(BDIR)/.f

$(BDIR)/%.o: src/%.cpp  | $(BDIR)/.f
	$(CXX) $(CXXFLAGS) -I. -Isrc -DGTEST_HAS_PTHREAD=1 -pthread -c -o $@ $<

# $(BDIR)/%: $$(@D)/.f

$(BDIR)/.f:
	@mkdir -p $(dir $@)
	@touch $@

.PRECIOUS: %/.f

cmake:
	mkdir -p cbuild
	(cd cbuild ; CXX=/usr/lib64/ccache/clang++ CC=/usr/lib64/ccache/clang cmake .. ; CXX=/usr/lib64/ccache/clang++ CC=/usr/lib64/ccache/clang make -j )

reformat:
	@perl -i -pe 's/	/    /g' $$(find -name '*.h' -o -name '*.cpp')
	@for f in $$(find -name '*.h' -o -name '*.cpp') ; do echo $$f ; clang-format -style=file -i $$f ; done
	@perl -i -pe 's/\s\s*$$/\n/g' $$(find -name '*.h' -o -name '*.cpp')

pdfs:
	@mkdir pdfs
	@for f in $$(find -name '*.h' -o -name '*.cpp') ; do enscript -qh2Gr -Ec --color=1 -p - -b '$n|%W|Page $% of $=' --highlight -t -$$f $$f | ps2pdf12 - - > pdfs/$$(echo "$$f" |sed -e 's/\.\///' |tr / _ ).pdf; done

clean:
	@rm -rf cbuild build pdfs
	@rm -f *.o *.d *~ core.* *.s *.ii *.bc *.ltrans.out ./*.res $(TARGETS) $$(find -name '*.pdf') $$(find -name '*.o')

tarball: clean
	@ D=$$(basename $$(pwd)); cd ..; X=$$(hostname)-$$(date '+%Y%m%d-%H%M%S'); ln -s $$D $$D-$$X; tar cf - $$( find $$D-$$X/ -type f |egrep -v '(./.git/|/cmake-build-debug)' |LANG=C sort ) | xz -9 > $$D-$$X.tar.xz ; rm $$D-$$X
