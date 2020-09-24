
struct temp_control_handle_t
{
    int relay_warming_pin;
    int relay_cooling_pin;
    bool warming_on;
    bool cooling_on;
};

void temp_control_configure(struct temp_control_handle_t *temp_control_handle);

void temp_control_perform(struct temp_control_handle_t *temp_control_handle);

void temp_control_set_warming(struct temp_control_handle_t *temp_control_handle);

void temp_control_set_cooling(struct temp_control_handle_t *temp_control_handle);

void temp_control_set_off(struct temp_control_handle_t *temp_control_handle);
