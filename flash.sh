# Script to flash esp32 sniffer
# Command: 
# >> sudo ./flash.sh -p /dev/ttyUSB0 -f esp32-freeRTOS2.ino.merged.bin
# To also provision a PSK after flashing:
# >> sudo ./flash.sh -p /dev/ttyUSB0 -f esp32-freeRTOS2.ino.merged.bin -k <psk>

filename=esp32-freeRTOS2.ino.merged.bin
port=/dev/ttyUSB0
psk=""

while [ "$#" -gt 0 ]; do
  case "$1" in
  	-h|--help)
      echo "args:
      	-p/--port		output port indetification
      	-f/--filename		name of file to be flashed inside current dir
      	-k/--psk		PSK to send to device over serial after flashing"
      exit 0
      ;;
    -p|--port)
      port="$2"
      echo "home dir set: ${port}"
      shift 2
      ;;
    -f|--filename)
      filename="$2"
      echo "filename: $filename"
      shift 2
      ;;
    -k|--psk)
      psk="$2"
      echo "psk provided"
      shift 2
      ;;
    *)
      echo "Unknown parameter: $1"
      exit 1
      ;;
  esac
done

#sudo esptool.py --port ${port} read_mac
sudo esptool --port ${port} erase_flash 
sudo esptool --port ${port} --baud 460800 write-flash 0x0 ${filename}

if [ -n "$psk" ]; then
  echo "Waiting for device to boot before sending PSK..."
  sleep 6
  python3 - "$port" "$psk" <<'PYEOF'
import sys, time

port = sys.argv[1]
psk  = sys.argv[2]

try:
    import serial
except ImportError:
    print("pyserial not found. Install it with: pip3 install pyserial")
    sys.exit(1)

s = serial.Serial(port, 115200, timeout=5)
time.sleep(2)  # allow any boot log to flush
cmd = '{"psk":"' + psk + '"}\n'
s.write(cmd.encode())
s.flush()
time.sleep(1)
response = s.read(s.in_waiting or 1).decode(errors='replace').strip()
s.close()

if response:
    print("Device response: " + response)
else:
    print("No response from device (PSK may still have been saved).")
PYEOF
fi
