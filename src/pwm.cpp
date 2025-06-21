
#include <stdio.h>
#include <unistd.h>
#include <chrono>
#include "pwm.h"

#define PWM_SYSFS "/sys/class/pwm/pwmchip0/"
#define PWM_PATH0 "/sys/class/pwm/pwmchip0/pwm0/"
#define PWM_PATH1 "/sys/class/pwm/pwmchip0/pwm1/"

// The purpose of this is that we can use the PWM ports of the Raspberry
// The idea comes from: https://gist.github.com/Gadgetoid/b92ad3db06ff8c264eef2abf0e09d569
// That is, first create a dts file named "pwm-pi5-overlay.dts" (part of the src directory),
// then compile it with dtc using the command:
// dtc -I dts -O dtb -o pwm-pi5.dtbo pwm-pi5-overlay.dts
// then install the resulting dtbo file with:
// sudo cp pwm-pi5.dtbo /boot/firmware/overlays/
// then edit the the Raspberry's /boot/firmware/config.txt file by adding the line
// dtoverlay=pwm-pi5
// (To edit the config.txt file is tricky - I used "sudo vi")
// Then, you are ready to use these methods below
// These are currently not used - will be used in a future version

PWM::PWM()
{
    // nothing to do
}

bool PWM::Init(const char *period_ns, const char *duty_cycle_ns)
{
    int chan = 0;
    bool ret = false;

    if (!ret)
        ret = this->WriteSYS(PWM_SYSFS "export", "0"); // export PWM channel (0)

    if (!ret)
        ret = this->WriteSYS(PWM_SYSFS "export", "1"); // export PWM channel (1)

    usleep(100000); // sleep for 100ms

    if (!ret)
        ret = this->WriteSYS(PWM_PATH0 "period", period_ns); // set period (1ms=1KHz)
    if (!ret)
        ret = this->WriteSYS(PWM_PATH0 "duty_cycle", duty_cycle_ns); // set duty_cycle
    if (!ret)
        ret = this->WriteSYS(PWM_PATH0 "enable", "1"); // enable

    if (!ret)
        ret = this->WriteSYS(PWM_PATH1 "period", "50000"); // set period (just an example)
    if (!ret)
        ret = this->WriteSYS(PWM_PATH1 "duty_cycle", "25000"); // set duty_cycle (just an example)
    if (!ret)
        ret = this->WriteSYS(PWM_PATH1 "enable", "1"); // enable

    return ret;
}

bool PWM::WriteSYS(const char filename[], const char value[])
{
    bool ret = false;
    FILE *fp;                  // create a file pointer fp
    fp = fopen(filename, "w"); // open file for writing
    if (fp != NULL)
    {
        // printf("PWM::WriteSYS(): Writing to [%s] value=[%s]\n", filename, value);
        fprintf(fp, "%s", value); // send the value to the file
        fclose(fp);               // close the file using fp
    }
    else
    {
        ret = true;
        printf("ERROR: %s(): can't open file:%s\n", __func__, filename);
    }

    return ret;
}
