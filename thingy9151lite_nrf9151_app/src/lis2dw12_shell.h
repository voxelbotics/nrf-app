#ifdef CONFIG_LIS2DW12_TRIGGER

extern int lis2dw12_trig_cnt;

void lis2dw12_trigger_handler(const struct device *dev,
				     const struct sensor_trigger *trig);
#endif
