#!/bin/bash

FILE="bluetracker"
DIR="$HOME/.bluetracker"

# Find the path of the ble.c file
PATH_C=$(find "$HOME" -name ble.c -exec dirname {} \; 2>/dev/null)

# Check if the ble.c file was found
if [[ -z $PATH_C ]]; then
    echo "Error: The 'ble.c' file was not found in the \$HOME directory."
    exit 1
fi

# Create the initialization script
echo "Creating the initialization script..."
echo "#!/bin/bash" > $FILE
echo "sudo $DIR/bluetracker.out \"\$@\"" >> $FILE

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
