# ir-sensor-monitor

Python script used to read the state from an HC-SR501 Passive Infrared sensor

## Components

### HC-SR501 Sensor
<img src="./images/sensor.jpg" style="max-width:150px" />

### State monitor `ism.py` in Python

* Read states from the sensor;
* Notify downstream application by calling RESTful APIs;
* Record detection events to a SQLite database.

### Detection records visualizer `q-visualizer` in C++/Qt

* Read detection records and plot a linechart in C++/Qt
