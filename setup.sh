#!/bin/bash


# Ensure the script is executed with root privileges
if [ "$EUID" -ne 0 ]; then
  echo "This script must be run as root. Please use 'sudo' to execute it."
  exec sudo "$0" "$@"
fi


HOME_DIR=$(eval echo "~$SUDO_USER")
DIR="$HOME_DIR/.bluetracker"
FILE="bluetracker"
PATH_C=$(dirname "$(realpath "$0")")


# Check if the ble.c file was found
if [[ ! -e "$PATH_C/ble.c" ]]; then
    echo "Error: The 'ble.c' file was not found in the \$HOME directory."
    exit 1
fi


if ! sudo apt install build-essential libbluetooth-dev > /dev/null 2>&1; then
    echo "Error: it was not possible to install the dependencies"
fi


# Create the initialization script
echo "Creating the initialization script..."
echo "#!/bin/bash" > $FILE
echo "if [ \"\$EUID\" -ne 0 ]; then" >> $FILE
echo "  exec sudo \"\$0\" \"\$@\"" >> $FILE
echo "fi" >> $FILE
echo "HOME_DIR=\$(eval echo "~\$SUDO_USER")" >> $FILE
echo "sudo \$HOME_DIR/bluetracker.out \"\$@\"" >> $FILE


# Move the script to /usr/bin
echo "Moving the script to /usr/bin..."
sudo mv $FILE /usr/bin/
sudo chmod +x /usr/bin/$FILE


# Create the program directory
echo "Creating the directory $DIR..."
mkdir -p "$DIR"


# Copy the LICENSE file if it exists
if [[ -f "$PATH_C/LICENSE" ]]; then
    echo "Copying the LICENSE file to $DIR..."
    cp "$PATH_C/LICENSE" "$DIR/"
else
    echo "Warning: The LICENSE file was not found."
fi


# Compile the code
echo "Compiling the code..."
gcc "$PATH_C/ble.c" -o "$DIR/bluetracker.out" -lbluetooth -lm


# Check if compilation was successful
if [[ $? -ne 0 ]]; then
    echo "Error during code compilation."
    exit 1
fi

echo "INSTALLATION COMPLETED"
