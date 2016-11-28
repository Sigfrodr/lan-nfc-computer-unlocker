# LaNCer (lan-nfc-computer-unlocker)

Lancer is a client/server nfc computer unlocker. 

## Architecture

* One or several Arduino(s) can be used as client(s).
* There is a centralized user management on the server.
* When a user scan a nfc tag on the Arduino, an access request is made to the server via a webservice.
* If the tag is recognized, then the password is entered via the Arduino keyboard (the computer is locked first to avoid writing the password anywhere).

## Warning

The encryption used is weak (xor + base64), plus the card id (which can be falsifiable) is used to identify the user.
Don't use this in a production environement.

## Notes

* If there is a user's switch on a computer, the new account must be selected before scanning the tag, otherwiser the wrong account will be used.
* The unlocker part can easily be customized to unlock more than only computers (doors, etc). In this case, the Leonardo is not a requirement (it is only for the emulated keyboard).

# Hardware requirements

* Arduino Leonardo
* Adafruit PN532 shield
* Ethernet shield

# Software requirements

* Apache, MySQL, PHP
* Composer
* Arduino IDE with following libraries: adafruit/Adafruit-PN532, bblanchon/ArduinoJson, agdl/Base64

# Installation

* Initialize MySQL DB with provided script.
* Server: use Composer to install dependencies. Configure DB and encryption key in src/settings.php.
* Client: configure server, subfolder, encryption key (must be the same as the key on the server) and OS. Upload the code onto the Arduino.

# Usage

* Add a user: grab the user's NFC card id via the serial console on the Arduino IDE. Create a new user with this card id and his password on the server's admin interface.
