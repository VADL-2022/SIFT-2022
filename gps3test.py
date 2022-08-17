from gps3 import agps3
gps_socket = agps3.GPSDSocket()
data_stream = agps3.Datastream()
gps_socket.connect()
gps_socket.watch()
for new_data in gps_socket:
	if new_data:
		data_stream.unpack(new_data)
		print('alt=', data_stream.alt)
		print('lat=', data_stream.lat)
		print('lon=', data_stream.lon)
