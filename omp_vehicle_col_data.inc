#if defined _omp_vehicle_col_data_included
    #endinput
#endif
#define _omp_vehicle_col_data_included

#define VEHICLE_COL_MIN_MODEL_ID 400
#define VEHICLE_COL_MAX_MODEL_ID 611
#define VEHICLE_COL_MODEL_COUNT (VEHICLE_COL_MAX_MODEL_ID - VEHICLE_COL_MIN_MODEL_ID + 1)
#define VEHICLE_COL_FORMAT_VERSION 1
#define VEHICLE_COL_MAX_LINE_LENGTH 512
#define VEHICLE_COL_MAX_FIELD_LENGTH 64
#define VEHICLE_COL_CSV_FIELD_COUNT 12

enum E_VEHICLE_COL_DATA {
    bool:VEH_COL_HAS_DATA,
    Float:VEH_COL_MIN_X,
    Float:VEH_COL_MIN_Y,
    Float:VEH_COL_MIN_Z,
    Float:VEH_COL_MAX_X,
    Float:VEH_COL_MAX_Y,
    Float:VEH_COL_MAX_Z,
    Float:VEH_COL_SPHERE_X,
    Float:VEH_COL_SPHERE_Y,
    Float:VEH_COL_SPHERE_Z,
    Float:VEH_COL_SPHERE_RADIUS
};

/*
* --------------------------------------------------------------------------
* INTERNAL FUNCTIONS
* --------------------------------------------------------------------------
*/

static gVehicleColData[VEHICLE_COL_MODEL_COUNT][E_VEHICLE_COL_DATA];
static bool:gVehicleColDataLoaded;

static stock bool:VehicleCol_IsValidModel(modelid) {
    if(modelid >= VEHICLE_COL_MIN_MODEL_ID && modelid <= VEHICLE_COL_MAX_MODEL_ID)
        return true;
    return false;
}

static stock VehicleCol_GetIndex(modelid) {
    return modelid - VEHICLE_COL_MIN_MODEL_ID;
}

static stock VehicleCol_TrimLine(string[]) {
    new length = strlen(string);

    while(length > 0) {
        new character = string[length - 1];

        if(character != '\r' && character != '\n')
            break;

        string[length - 1] = '\0';
        length--;
    }

    return length;
}

static stock VehicleCol_CountCSVFields(const line[]){
    if(line[0] == '\0')
        return 0;

    new fields = 1;

    for(new i = 0; line[i] != '\0'; i++) {
        if(line[i] == ',')
            fields++;
    }

    return fields;
}

static stock VehicleCol_ReadCSVField(const line[], start, output[], output_size) {
    new position = start;
    new output_position = 0;

    while(line[position] != '\0' && line[position] != ',') {
        if(output_position < output_size - 1) {
            output[output_position++] = line[position];
        }

        position++;
    }

    output[output_position] = '\0';

    /*
    * Skip the comma
    */
    if(line[position] == ',')
        position++;

    return position;
}

static stock bool:VehicleCol_ClearData() {
    for (new index = 0; index < VEHICLE_COL_MODEL_COUNT; index++) {
        gVehicleColData[index][VEH_COL_HAS_DATA] = false;
        gVehicleColData[index][VEH_COL_MIN_X] = 0.0;
        gVehicleColData[index][VEH_COL_MIN_Y] = 0.0;
        gVehicleColData[index][VEH_COL_MIN_Z] = 0.0;
        gVehicleColData[index][VEH_COL_MAX_X] = 0.0;
        gVehicleColData[index][VEH_COL_MAX_Y] = 0.0;
        gVehicleColData[index][VEH_COL_MAX_Z] = 0.0;
        gVehicleColData[index][VEH_COL_SPHERE_X] = 0.0;
        gVehicleColData[index][VEH_COL_SPHERE_Y] = 0.0;
        gVehicleColData[index][VEH_COL_SPHERE_Z] = 0.0;
        gVehicleColData[index][VEH_COL_SPHERE_RADIUS] = 0.0;
    }
    gVehicleColDataLoaded = false;
    return true;
}

/*
* --------------------------------------------------------------------------
* USABLE FUNCTIONS
* --------------------------------------------------------------------------
*/

/*
* Loads the CSV file
*/
stock bool:LoadVehicleColData(const filename[] = "vehicle_col_data.csv") {
    /*
    * Model 594 has no col data, but should exist in the CSV file either way
    */
    new bool:seen[VEHICLE_COL_MODEL_COUNT];

    VehicleCol_ClearData();

    new File:file = fopen(filename, io_read);

    if(!file) {
        printf("[omp-vehicle-col-data] ERROR: Could not open '%s'!", filename);
        return false;
    }

    new line[VEHICLE_COL_MAX_LINE_LENGTH];
    new line_number = 0;

    if(!fread(file, line)) {
        printf("[omp-vehicle-col-data] ERROR: Data file is empty!");
        fclose(file);
        return false;
    }

    line_number++;

    VehicleCol_TrimLine(line);

    new expected_marker[64];

    /*
    * Exit if incorrect file version is found
    */
    format(expected_marker, sizeof expected_marker, "# omp-vehicle-col-data,%d", VEHICLE_COL_FORMAT_VERSION);
    if(strcmp(line, expected_marker, false) != 0) {
        printf("[omp-vehicle-col-data] ERROR: Unsupported marker version on line %d, expected '%s', found '%s'", line_number, expected_marker, line);
        fclose(file);
        return false;
    }

    if(!fread(file, line)) {
        printf("[omp-vehicle-col-data] ERROR: Missing CSV header!");
        fclose(file);
        return false;
    }

    line_number++;
    VehicleCol_TrimLine(line);

    /*
    * Exit if the CSV header is not valid
    */
    static const expected_header[] = "model_id,has_col,min_x,min_y,min_z,max_x,max_y,max_z,sphere_x,sphere_y,sphere_z,sphere_radius";
    if(strcmp(line, expected_header, false) != 0) {
        printf("[omp-vehicle-col-data] ERROR: Invalid CSV header on line %d, expected '%s', found '%s'", line_number, expected_header, line);
        fclose(file);
        return false;
    }

    new loaded_rows = 0;
    new models_with_col = 0;
    new models_without_col = 0;

    while(fread(file, line)) {
        line_number++;

        VehicleCol_TrimLine(line);

        /*
        * Skip empty lines
        */
        if(line[0] == '\0')
            continue;

        /*
        * Check if field count matches
        */
        new field_count = VehicleCol_CountCSVFields(line);
        if(field_count != VEHICLE_COL_CSV_FIELD_COUNT) {
            printf("[omp-vehicle-col-data] ERROR: Invalid field count on line %d, expected '%d', found '%d'", line_number, VEHICLE_COL_CSV_FIELD_COUNT, field_count);
            fclose(file);
            VehicleCol_ClearData();
            return false;
        }

        new field[VEHICLE_COL_MAX_FIELD_LENGTH];
        new position = 0;

        /*
        * model_id
        */
        position = VehicleCol_ReadCSVField(line, position, field, sizeof field);
        new modelid = strval(field);

        if(!VehicleCol_IsValidModel(modelid)) {
            printf("[omp-vehicle-col-data] ERROR: Invalid model ID '%d' on line %d.", modelid, line_number);
            fclose(file);
            VehicleCol_ClearData();
            return false;
        }

        new index = VehicleCol_GetIndex(modelid);
        if(seen[index]) {
            printf("[omp-vehicle-col-data] ERROR: Duplicate model ID '%d' on line %d.", modelid, line_number);
            fclose(file);
            VehicleCol_ClearData();
            return false;
        }

        /*
        * has_col
        */
        position = VehicleCol_ReadCSVField(line, position, field, sizeof field);
        new has_col = strval(field);
        if(has_col != 0 && has_col != 1) {
            printf("[omp-vehicle-col-data] ERROR: Invalid has_col value '%s' on line %d!", field, line_number);
            fclose(file);
            VehicleCol_ClearData();
            return false;
        }
        gVehicleColData[index][VEH_COL_HAS_DATA] = (has_col == 1);

        /*
        * min_x
        */
        position = VehicleCol_ReadCSVField(line, position, field, sizeof field);
        gVehicleColData[index][VEH_COL_MIN_X] = floatstr(field);

        /*
        * min_y
        */
        position = VehicleCol_ReadCSVField(line, position, field, sizeof field);
        gVehicleColData[index][VEH_COL_MIN_Y] = floatstr(field);

        /*
        * min_z
        */
        position = VehicleCol_ReadCSVField(line, position, field, sizeof field);
        gVehicleColData[index][VEH_COL_MIN_Z] = floatstr(field);

        /*
        * max_x
        */
        position = VehicleCol_ReadCSVField(line, position, field, sizeof field);
        gVehicleColData[index][VEH_COL_MAX_X] = floatstr(field);

        /*
        * max_y
        */
        position = VehicleCol_ReadCSVField(line, position, field, sizeof field);
        gVehicleColData[index][VEH_COL_MAX_Y] = floatstr(field);

        /*
        * max_z
        */
        position = VehicleCol_ReadCSVField(line, position, field, sizeof field);
        gVehicleColData[index][VEH_COL_MAX_Z] = floatstr(field);

        /*
        * sphere_x
        */
        position = VehicleCol_ReadCSVField(line, position, field, sizeof field);
        gVehicleColData[index][VEH_COL_SPHERE_X] = floatstr(field);

        /*
        * sphere_y
        */
        position = VehicleCol_ReadCSVField(line, position, field, sizeof field);
        gVehicleColData[index][VEH_COL_SPHERE_Y] = floatstr(field);

        /*
        * sphere_z
        */
        position = VehicleCol_ReadCSVField(line, position, field, sizeof field);
        gVehicleColData[index][VEH_COL_SPHERE_Z] = floatstr(field);

        /*
        * sphere_radius
        */
        VehicleCol_ReadCSVField(line, position, field, sizeof field);
        gVehicleColData[index][VEH_COL_SPHERE_RADIUS] = floatstr(field);

        seen[index] = true;
        loaded_rows++;

        if(gVehicleColData[index][VEH_COL_HAS_DATA])
            models_with_col++;
        else
            models_without_col++;
    }

    fclose(file);

    /*
    * Check if all models actually exist
    */
    for (new index = 0; index < VEHICLE_COL_MODEL_COUNT; index++) {
        if(!seen[index]) {
            printf("[omp-vehicle-col-data] ERROR: Model ID %d is missing from the data file.", index + VEHICLE_COL_MIN_MODEL_ID);

            VehicleCol_ClearData();

            return false;
        }
    }


    gVehicleColDataLoaded = true;
    printf("[omp-vehicle-col-data] Loaded %d vehicle models (%d with COL, %d without COL).", loaded_rows, models_with_col, models_without_col);
    return true;
}


stock bool:IsVehicleColDataLoaded() {
    return gVehicleColDataLoaded;
}

stock bool:IsVehicleModelColDataAvailable(modelid) {
    if(!gVehicleColDataLoaded)
        return false;

    if(!VehicleCol_IsValidModel(modelid))
        return false;

    return gVehicleColData[VehicleCol_GetIndex(modelid)][VEH_COL_HAS_DATA];
}

stock bool:GetVehicleModelColMin(modelid, &Float:x, &Float:y, &Float:z) {
    if(!IsVehicleModelColDataAvailable(modelid))
        return false;

    new index = VehicleCol_GetIndex(modelid);

    x = gVehicleColData[index][VEH_COL_MIN_X];
    y = gVehicleColData[index][VEH_COL_MIN_Y];
    z = gVehicleColData[index][VEH_COL_MIN_Z];

    return true;
}

stock bool:GetVehicleModelColMax(modelid, &Float:x, &Float:y, &Float:z) {
    if(!IsVehicleModelColDataAvailable(modelid))
        return false;

    new index = VehicleCol_GetIndex(modelid);

    x = gVehicleColData[index][VEH_COL_MAX_X];
    y = gVehicleColData[index][VEH_COL_MAX_Y];
    z = gVehicleColData[index][VEH_COL_MAX_Z];

    return true;
}

stock bool:GetVehicleModelColSphere(modelid, &Float:x, &Float:y, &Float:z, &Float:radius) {
    if(!IsVehicleModelColDataAvailable(modelid))
        return false;

    new index = VehicleCol_GetIndex(modelid);

    x = gVehicleColData[index][VEH_COL_SPHERE_X];
    y = gVehicleColData[index][VEH_COL_SPHERE_Y];
    z = gVehicleColData[index][VEH_COL_SPHERE_Z];

    radius = gVehicleColData[index][VEH_COL_SPHERE_RADIUS];

    return true;
}

stock bool:GetVehicleModelHeightAboveRoad(modelid, &Float:height) {
    if(!IsVehicleModelColDataAvailable(modelid))
        return false;

    height = -gVehicleColData[VehicleCol_GetIndex(modelid)][VEH_COL_MIN_Z];

    return true;
}

stock bool:GetVehicleModelColSize(modelid, &Float:x, &Float:y, &Float:z) {
    if(!IsVehicleModelColDataAvailable(modelid))
        return false;

    new index = VehicleCol_GetIndex(modelid);

    x = gVehicleColData[index][VEH_COL_MAX_X] - gVehicleColData[index][VEH_COL_MIN_X];
    y = gVehicleColData[index][VEH_COL_MAX_Y] - gVehicleColData[index][VEH_COL_MIN_Y];
    z = gVehicleColData[index][VEH_COL_MAX_Z] - gVehicleColData[index][VEH_COL_MIN_Z];

    return true;
}
