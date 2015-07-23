#ifndef _CAMEARA_H_
#define _CAMEARA_H_

#define END_MARKER {0xff, 0xff}

struct reg_val {
    uint8_t reg;
    uint8_t val;
};

struct camera_sensor_data {
    int bus;   /* controller number */
    int chip;  /* device address */
    int clk_rate;
    struct reg_val *reg_val;

    /* callbacks */
    int (*power_enable)(bool);
    int (*set_clock)(int);
    void (*setup_io)(void);
};

/**
 * return camera sensor logical id
 */
int camera_register_sensor(struct camera_sensor_data *data);
int camera_sensor_run(int id);

#endif
