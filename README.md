# TAGE_BP
This is an implementation of the TAGE branch predictor with a bit budget of 64Kbits.

The main code for the predictor is at `src/predictor.cpp` and `src/predictor.h`.

In addition to TAGE, the code contains implementations of Gshare and a tournament branch predictor based on the Alpha 21264.

For testing, 4 traces are present in the `traces` directory.


# Usage
Run `make all` in the `src` folder to compile the code.

To test the branch predictor run

``
bunzip2 -kc /path/to/trace | ./predictor --predictor_type
``

# Performance
The below table reports the number of misprediction per 1K branches for the 3 branch predictors when tested with each trace.

| Predictor      | U1_Blender | U2_Leela | U3_GCC | U4_Cam4 | Average   |
|----------------|------------|----------|--------|---------|-----------|
| Gshare         | 33.654     | 101.729  | 19.608 | 10.030  | 41.25525  |
| Tournament     | 29.379     | 93.371   | 14.086 | 6.486   | 35.8305   |
| Custom (TAGE)  | 30.101     | 82.406   | 6.166  | 7.376   | 31.51225  |
