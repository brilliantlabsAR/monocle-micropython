#!/usr/bin/awk -f

# This tool generate C code for driver_battery.c, for estimating the
# battery discharge curve out of a list of voltage values.
# 
# The input file has one mV value per line.
# For instance, a line with "4232" represents 4.232 V.
# 
# The first value must be taken at full charge, while the power cable
# is disconnected.
#
# The last value must be taken once the Bluetooth connection disconnects
# due to low power.
#
# The intermediate values must be taken at a regular interval, any duration
# as long as it is constant, for instance from battery_timer_handler().
#
# The output will be C code with 11 values for steps from 0% to 100% with
# a 10% step in-between.

BEGIN {
    steps = 10
}

{
    V[NR] = $0
}

END {
    printf "static float battery_discharge_curve["steps" + 1] = {\n    %.2f, ", V[1] / 1000

    for (st = 0; st < steps - 1; st++) {
        i = int(st * NR / 9)
        sum = 0
        n = int(NR / 9) + 1
        for (ii = 0; ii < n; ii++) {
            sum = sum + V[i + ii]
        }
        printf "%.2f, ", sum / n / 1000
    }

    printf "%.2f\n};\n", V[NR] / 1000
}
