# How to use it

1) Clone the repo
```
git clone https://github.com/wg1337/omp-vehicle-col-data
```
2) Go to /tools directory and install the required library
```
cd tools/
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install 'git+https://github.com/Hancapo/rwfury.git'
```
3) Copy the "gta3.img" to the tools/ folder
```
cp /path/to/gta_sa/models/gta3.img .
```
4) Copy "vehicles.ide" to the tools/ folder
```
cp /path/to/gta_sa/data/vehicles.ide .
```
5) Extract .DFF files
```
python extract_vehicles.py gta3.img vehicles.ide extracted-vehicles
```
5) Generate CSV files from the extracted .DFF files
```
python generate_vehicle_col_data.py vehicles.ide extracted-vehicles vehicle_col_data.csv
```
6) Copy the CSV file to your scriptfiles/ folder
```
cp vehicle_col_data.csv /path/to/open.mp/scriptfiles/
```
7) Include the .inc file into your gamemode/filterscript
```
#include <omp_vehicle_col_data>
```
8) Load the CSv file, for example:
```
public OnFilterScriptInit() {
    ....
    if (!LoadVehicleColData("vehicle_col_data.csv")){
        print("[omp-vehicle-col-data] Failed to load vehicle collision data.");
        return false;
    }
    ....
    return true;
}
```
9) Use the included functions to retrieve data, for example:
```
new Float:VehicleHeightFromGround = 0.0;
GetVehicleModelHeightAboveRoad(modelid, VehicleHeightFromGround);
...
NPC_Move(npcid, x, y, z + VehicleHeightFromGround, NPC_MOVE_TYPE_DRIVE, 1.0);
```


# Functions
```
bool:LoadVehicleColData(const filename[] = "vehicle_col_data.csv") - loads the CSV file
bool:IsVehicleColDataLoaded() - checks if CSV file has been loaded successfully
bool:IsVehicleModelColDataAvailable(modelid) - checks if the vehicle has collision data
bool:GetVehicleModelColMin(modelid, &Float:x, &Float:y, &Float:z) - gets the min collision box coordinates (not very useful by itself)
bool:GetVehicleModelColMax(modelid, &Float:x, &Float:y, &Float:z) - gets the max collision box coordinates (not very useful by itself)
bool:GetVehicleModelColSize(modelid, &Float:x, &Float:y, &Float:z) - gets the vehicle's collision box size
bool:GetVehicleModelHeightAboveRoad(modelid, &Float:height) - gets the vehicle's height, including the wheel size, useful for determining how high should a car be spawned to be on the ground
```
