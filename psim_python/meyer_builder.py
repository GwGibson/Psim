from psim import pre_builts


# Common settings
sim_time = 30 # Might want to scale this to length but 90 should be fine more than enough
num_measurements = 1000
num_phonons = 500000
t_high = 310 # Temperature of heat source
t_low = 290 # Temperature of heat sink
t_init = 300 # All cells will start at this temperature
t_eq = (t_high + t_low) / 2 # Very important this is reasonable for 'no-reset' runs
cell_x_len = 10 
cell_y_len = 50 

lengths = (1000, 2000) # System lengths that you want to generate .json files for
for length in lengths:
    num_cells = length // cell_x_len

    # Default is a steady-state simulation with fully specular surfaces
    b = pre_builts.simple_linear(num_cells, t_high, t_low, t_init, t_eq, cell_x_len, cell_y_len)
    b.setSimTime(sim_time)
    b.setMeasurements(num_measurements)
    b.setNumPhonons(num_phonons)

    filename = f'linear_{length}_{num_cells}.json'
    b.export(filename)
