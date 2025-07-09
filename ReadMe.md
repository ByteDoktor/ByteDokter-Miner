# Silent XMR Miner

This builder will output an exe with your custom settings this payload should only ran on machine you're authorised to infect.

## Disclaimer
THIS SOFTWARE IS INTENDED FOR EDUCATIONAL USE ONLY AND IS NOT SUITABLE FOR ANY ACTUAL HACKING OR ILLEGAL ACTIVITIES. THE AUTHOR(S) SHALL NOT BE LIABLE FOR ANY INDIRECT, SPECIAL, INCIDENTAL, TORTIOUS, OR CONSEQUENTIAL DAMAGES THAT MAY ARISE FROM YOUR USE OF THIS SOFTWARE.

BY ACCESSING AND USING THIS SOFTWARE, YOU ACKNOWLEDGE THAT IT IS EXCLUSIVELY FOR EDUCATIONAL PURPOSES TO DEMONSTRATE MODERN MALWARE TECHNIQUES. YOU AGREE NOT TO UTILIZE IT FOR ANY HARMFUL OR ILLEGAL INTENTS.

PLEASE NOTE: THIS PROJECT IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED. YOU ARE RESPONSIBLE FOR ENSURING THAT YOUR USE COMPLIES WITH ALL APPLICABLE LAWS AND REGULATIONS WHERE YOU RESIDE.

## Images

### Pool Tab
![Pool Tab](/Images/PoolTab.png?raw=true "PoolTabScreen")

### Stealth Tab
![Stealth Tab](/Images/StealthTab.png?raw=true "StealthTabScreen")

### Misc Tab
![Misc Tab](/Images/MiscTab.png?raw=true "MiscTabScreen")


## Installation

Download code and open the .SLN file with visual studio and build the project or download the release version.


## Main Features

- Watchdog - This will spawn a separate process that will watch and restart the miner automatically if needed.
- Stealth - Will pause the miner if the user launches any programs on the specified list.
- Stealth On Fullscreen - The miner will go into stealth mode when the user is in Fullscreen.
- Self Delete - The program will delete itself after running.
- Load from Url - The miner will be loaded from a url provided.
- Add To Start Up - This will launch the miner autoically upon restart.
- Add Defender Exclusion - This will add an Exclusion to all paths related to the miner.
- Remote Configuration - You can modify the settings of the miner without have to rebuild the payload.
- Idle Mining - This will dynamically adjust the resource usage based on if the user is active or idle.
- Process Injection - The miner will injected into a specifed process using [Process Hollowing](https://hshrzd.wordpress.com/2025/01/27/process-hollowing-on-windows-11-24h2/)
 
 ## Techniques & Tools used
 - [Process Hollowing](https://hshrzd.wordpress.com/2025/01/27/process-hollowing-on-windows-11-24h2/) -  A common injection technique to run code as another process.
 - [Lazy Importer](https://github.com/JustasMasiulis/lazy_importer) - library for importing functions from dlls in a hidden, reverse engineer unfriendly way.
 - [Tiny-AES](https://github.com/kokke/tiny-AES-c)- This library was used for encryption and decryption. Its small lightweight and portable.
 - [Obfuscate](https://github.com/adamyaxley/Obfuscate) - This library was used to obfucate and hide strings in the program.

## Contributing

Pull requests are welcome. For major changes, please open an issue first
to discuss what you would like to change.

Please make sure to update tests as appropriate.

## Authors and acknowledgment
The design for this project was inspired by [UnamSanctam](https://github.com/UnamSanctam)

## License
This project is licensed under the MIT License - see the [LICENSE](/LICENSE) file for details
