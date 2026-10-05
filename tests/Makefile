# Portable unit tests for the FastAcq core (protocol, store, DSP, session file).
# Linux/macOS: make test      Windows: build tests/FastAcqTests.vcxproj
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -I../FastAcq
SRC := test_main.cpp \
       ../FastAcq/ChirpStore.cpp ../FastAcq/ProtocolParser.cpp \
       ../FastAcq/Dsp/FftPlan.cpp ../FastAcq/Dsp/Window.cpp ../FastAcq/Dsp/Decimator.cpp \
       ../FastAcq/Dsp/Cfar.cpp ../FastAcq/Dsp/PeakFinder.cpp ../FastAcq/Dsp/RadarDsp.cpp \
       ../FastAcq/Dsp/ToneEstimator.cpp \
       ../FastAcq/Core/SessionFile.cpp ../FastAcq/Core/Export.cpp ../FastAcq/Core/ChirpGeometry.cpp

fastacq_tests: $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

test: fastacq_tests
	./fastacq_tests

clean:
	rm -f fastacq_tests *.o

.PHONY: test clean
