/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <chrono>

#include "xf_fintech_mc_example.hpp"

#include "xf_fintech_api.hpp"

using namespace xf::fintech;

// The MC European XCLBIN contains 4 kernels.  Each of these kernels implement
// an instance of the MC European engine.
//
// In this example, we run all 4 kernels simultaneously, providing each kernel
// with the data for DIFFERENT ASSET.
//
// We could pass large arrays of asset data and make a single call.  Within the
// MCEuropean object, this asset data
// would be processed in blocks of 4 (1 asset being assigned to 1 kernel, then
// the 4 kernels are executed together).
// However we would only be able to measure the overall time of processing the
// complete large array.
//
// To provide a better indication of performance, we will pass arrays of 4 sets
// of asset data.  This will allow us to
// see the performance of each invocation of the HW.

static OptionType baseOptionType = Put;

static const double baseStockPrice = 36.0;
static const double baseStrikePrice = 40.0;
static const double baseRiskFreeRate = 0.06;
static const double baseDividendYield = 0.0;
static const double baseVolatility = 0.20;
static const double baseTimeToMaturity = 1.0; /* in years */

static const double baseRequiredTolerance = 0.02;
static const double baseRequiredNumSamples = 16383;

/* The following variable is used to vary our input data for each run....*/
static const double varianceFactor = 0.001;

static double initialStockPrice[MCEuropean::NUM_KERNELS];
static double initialStrikePrice[MCEuropean::NUM_KERNELS];
static double initialRiskFreeRate[MCEuropean::NUM_KERNELS];
static double initialDividendYield[MCEuropean::NUM_KERNELS];
static double initialVolatility[MCEuropean::NUM_KERNELS];

static OptionType optionType[MCEuropean::NUM_KERNELS];
static double stockPrice[MCEuropean::NUM_KERNELS];
static double strikePrice[MCEuropean::NUM_KERNELS];
static double riskFreeRate[MCEuropean::NUM_KERNELS];
static double dividendYield[MCEuropean::NUM_KERNELS];
static double volatility[MCEuropean::NUM_KERNELS];
static double timeToMaturity[MCEuropean::NUM_KERNELS];
static double requiredTolerance[MCEuropean::NUM_KERNELS];
static double requiredNumSamples[MCEuropean::NUM_KERNELS];

static double optionPrice[MCEuropean::NUM_KERNELS];

static const unsigned int NUM_ITERATIONS = 250;

static void SetupParameters(void) {
    int i;

    for (i = 0; i < MCEuropean::NUM_KERNELS; i++) {
        optionType[i] = baseOptionType;
        initialStockPrice[i] = baseStockPrice + (i * 2);
        initialStrikePrice[i] = baseStrikePrice + (i * 2);
        initialRiskFreeRate[i] = baseRiskFreeRate + (i * 0.01);
        initialDividendYield[i] = baseDividendYield + (i * 0.01);
        initialVolatility[i] = baseVolatility + (i * 0.05);
        timeToMaturity[i] = baseTimeToMaturity;
        requiredTolerance[i] = baseRequiredTolerance;
        requiredNumSamples[i] = baseRequiredNumSamples;
    }
}

static void PrintParameters(void) {
    int i;

    printf("\n");
    printf("\n");
    printf("[XLNX] ==========\n");
    printf("[XLNX] Parameters\n");
    printf("[XLNX] ==========\n");
    printf(
        "[XLNX] "
        "+------------------+------------+------------+------------+------------+"
        "\n");
    printf(
        "[XLNX] | Parameter        |  Kernel 1  |  Kernel 2  |  Kernel 3  |  "
        "Kernel 4  |\n");
    printf(
        "[XLNX] "
        "+------------------+------------+------------+------------+------------+"
        "\n");

    printf("[XLNX] | Option Type      |");
    for (i = 0; i < MCEuropean::NUM_KERNELS; i++) {
        printf("  %-5s     |", Trace::optionTypeToString(optionType[i]));
    }
    printf("\n");

    printf("[XLNX] | Stock Price      |");
    for (i = 0; i < MCEuropean::NUM_KERNELS; i++) {
        printf("  %-8.4f  |", initialStockPrice[i]);
    }
    printf("\n");

    printf("[XLNX] | Strike Price     |");
    for (i = 0; i < MCEuropean::NUM_KERNELS; i++) {
        printf("  %-8.4f  |", initialStrikePrice[i]);
    }
    printf("\n");

    printf("[XLNX] | Risk Free Rate   |");
    for (i = 0; i < MCEuropean::NUM_KERNELS; i++) {
        printf("  %-8.4f  |", initialRiskFreeRate[i]);
    }
    printf("\n");

    printf("[XLNX] | Dividend Yield   |");
    for (i = 0; i < MCEuropean::NUM_KERNELS; i++) {
        printf("  %-8.4f  |", initialDividendYield[i]);
    }
    printf("\n");

    printf("[XLNX] | Volatility       |");
    for (i = 0; i < MCEuropean::NUM_KERNELS; i++) {
        printf("  %-8.4f  |", initialVolatility[i]);
    }
    printf("\n");

    printf("[XLNX] | Time To Maturity |");
    for (i = 0; i < MCEuropean::NUM_KERNELS; i++) {
        printf("  %-8.4f  |", timeToMaturity[i]);
    }
    printf("\n");

    printf("[XLNX] | Req. Tolerance   |");
    for (i = 0; i < MCEuropean::NUM_KERNELS; i++) {
        printf("  %-8.4f  |", requiredTolerance[i]);
    }
    printf("\n");

    printf(
        "[XLNX] "
        "+------------------+------------+------------+------------+------------+"
        "\n");

    printf("\n");

    printf("[XLNX] varianceFactor = %f * iteration\n", varianceFactor);

    printf("\n\n");
}

int MCDemoRunEuropeanMultiple1(Device* pChosenDevice, MCEuropean* pMCEuropean) {
    int retval = XLNX_OK;
    unsigned int i, j;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::chrono::time_point<std::chrono::high_resolution_clock> end;

    long long int duration;
    double durationPerAsset;

    printf("\n\n\n");

    printf(
        "[XLNX] "
        "***************************************************************\n");
    printf("[XLNX] Running MC EUROPEAN MULTI ASSET 1...\n");
    printf(
        "[XLNX] "
        "***************************************************************\n");

    //
    // Claim the device for our MCEuropean object...this will download the
    // required XCLBIN file (if needed)...
    //
    printf("[XLNX] mcEuropean trying to claim device...\n");

    start = std::chrono::high_resolution_clock::now();

    retval = pMCEuropean->claimDevice(pChosenDevice);

    end = std::chrono::high_resolution_clock::now();

    if (retval == XLNX_OK) {
        printf("[XLNX] Device setup time = %lld microseconds\n",
               (long long int)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
    } else {
        printf("[XLNX] ERROR- Failed to claim device - error = %d\n", retval);
    }

    if (retval == XLNX_OK) {
        SetupParameters();
        PrintParameters();
    }

    //
    // Run the model a few times...
    //
    if (retval == XLNX_OK) {
        printf("[XLNX] +-----------+-----------+----------------+---------------------+\n");
        printf("[XLNX] | Iteration | K1 Option | Execution Time | Avg. Time Per Asset |\n");
        printf("[XLNX] +-----------+-----------+----------------+---------------------+\n");

        for (i = 0; i < NUM_ITERATIONS; i++) {
            /* We will apply some variance to our data here so we are not cacheing any
             * values... */
            double variance = (1.0 + (varianceFactor * i));

            for (j = 0; j < MCEuropean::NUM_KERNELS; j++) {
                stockPrice[j] = initialStockPrice[j] * variance;
                strikePrice[j] = initialStrikePrice[j] * variance;
                riskFreeRate[j] = initialRiskFreeRate[j] * variance;
                dividendYield[j] = initialDividendYield[j] * variance;
                volatility[j] = initialVolatility[j] * variance;

                timeToMaturity[j] = baseTimeToMaturity;
                requiredTolerance[j] = baseRequiredTolerance;
            }

            retval = pMCEuropean->run(optionType, stockPrice, strikePrice, riskFreeRate, dividendYield, volatility,
                                      timeToMaturity, requiredTolerance, optionPrice, MCEuropean::NUM_KERNELS);

            duration = pMCEuropean->getLastRunTime();
            durationPerAsset = (double)duration / (double)MCEuropean::NUM_KERNELS;

            printf("[XLNX] | %9d | %9.4f  | %11lld us | %16.3f us |\n", i, optionPrice[0], duration, durationPerAsset);
        }

        printf("[XLNX] +-----------+-----------+----------------+---------------------+\n");
    }

    //
    // Release the device so another object can claim it...
    //
    printf("[XLNX] mcEuropean releasing device...\n");
    retval = pMCEuropean->releaseDevice();

    return retval;
}
