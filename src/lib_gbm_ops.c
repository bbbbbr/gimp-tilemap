//
// lib_gbm_ops.c
//

#include "lib_gbm_ops.h"


// TODO: remove once debugging is complete
void gbm_map_tiles_print(gbm_record * p_gbm) {

    uint16_t x, y;
    uint16_t index;

    gbm_tile_record tile;

    index = 0;
    for (y=0; y < p_gbm->map.height; y++) {
        for (x=0; x < p_gbm->map.width; x++)

            tile = gbm_map_tile_get_xy(p_gbm, x, y);
            index++;

            printf(" %d", tile.num);
            if ((index % p_gbm->map.width) == 0) printf("\n");

        }
}



gbm_tile_record gbm_map_tile_get_xy(gbm_record * p_gbm, uint16_t x, uint16_t y) {

    uint16_t index;

    gbm_tile_record tile;

    index = (x + (y * p_gbm->map.width)) * GBM_MAP_TILE_RECORD_SIZE;

    if ((index + 2) > p_gbm->map_tile_data.length_bytes) {
        tile.num = 0xFFFF;
        return tile;  // TODO: use/signal proper failure return code here
    }

    tile.flip_h = p_gbm->map_tile_data.records[index] & GBM_MAP_TILE_FLIP_H_BYTE;
    tile.flip_v = p_gbm->map_tile_data.records[index] & GBM_MAP_TILE_FLIP_V_BYTE;

    tile.num = ((uint16_t)p_gbm->map_tile_data.records[index+2] |
               ((uint16_t)p_gbm->map_tile_data.records[index+1] >> 8)) & GBM_MAP_TILE_NUM;

        printf(" %d", tile.num);
        if ((x % p_gbm->map.width) == 0) printf("\n");

    return tile;

}



uint32_t gbm_map_tile_set_xy(gbm_record * p_gbm, uint16_t x, uint16_t y, uint16_t tile_index) {

    uint32_t index;

    index = (x + (y * p_gbm->map.width)) * GBM_MAP_TILE_RECORD_SIZE;

    if ((index + 2) > p_gbm->map_tile_data.length_bytes) {
        return false;  // TODO: use/signal proper failure return code here
        printf("gbm_map_tile_set_xy: FAILED: %d, %d \n", x,y);
    }

    // Flip H & V (cgb)
    p_gbm->map_tile_data.records[index]  = 0;
    p_gbm->map_tile_data.records[index] |= 0; // TODO: GBM_MAP_TILE_FLIP_H_BYTE
    p_gbm->map_tile_data.records[index] |= 0; // TODO: GBM_MAP_TILE_FLIP_V_BYTE

    // Tile Num
    p_gbm->map_tile_data.records[index + 1] = (uint8_t)((tile_index & GBM_MAP_TILE_NUM) << 8);
    p_gbm->map_tile_data.records[index + 2] = (uint8_t) (tile_index & GBM_MAP_TILE_NUM);

        printf(" %d", tile_index);
        if ((x % p_gbm->map.width) == 0) printf("\n");

    return true;
}



int32_t gbm_convert_map_to_image(gbm_record * p_gbm, gbr_record * p_gbr, image_data * p_image) {

    uint16_t map_x, map_y;
    gbm_tile_record tile;
    int32_t status;

    status = true; // default to success

    // Init image
    p_image->bytes_per_pixel = 1; // TODO: MAKE a #define

    p_image->width  = p_gbm->map.width * p_gbr->tile_data.width;
    p_image->height = p_gbm->map.height * p_gbr->tile_data.height;

    // Calculate total image area based on
    // tile width and height, and number of tiles
    p_image->size = p_image->width * p_image->height * p_image->bytes_per_pixel;

    printf("gbm_convert_map_to_image: %dx%d @ size%d\n", p_image->width,
                                                         p_image->height,
                                                         p_image->size);

    // Allocate image buffer, free if needed beforehand
    if (p_image->p_img_data)
        free (p_image->p_img_data);

    p_image->p_img_data = malloc(p_image->size);


    // Create the image from tiles
    if (p_image->p_img_data) {

        // Loop through tile map, get tile number and copy respective tiles into image
        for (map_y=0; map_y < p_gbm->map.height; map_y++) {
            for (map_x=0; map_x < p_gbm->map.width; map_x++) {

                tile = gbm_map_tile_get_xy(p_gbm, map_x, map_y);

                if (status)
                    status = gbr_tile_copy_to_image(p_image,
                                                    p_gbr,
                                                    tile.num,
                                                    map_x, map_y);
            } // for .. map_x
        } // for.. map_y
    } else {
        // Signal failure if image pointer not allocated
        status = false;
    }


    // Return success
    return status;
}


int32_t gbm_convert_tilemap_buf_to_map(gbm_record * p_gbm, uint8_t * p_map_data, uint32_t map_data_count) {

    uint16_t map_x, map_y;

    printf("gbm_convert_image_to_map: %dx%d @ size=%d\n", p_gbm->map.width,
                                                          p_gbm->map.height,
                                                          map_data_count);

    // Bounds check for map data and counters
    if ((p_gbm->map.width * p_gbm->map.height) > map_data_count)
        return false;

    // Copy the tile data into the file_file image from tiles
    if (p_map_data) {

        // Loop through tile map, get tile number and copy respective tiles into image
        for (map_y=0; map_y < p_gbm->map.height; map_y++) {
            for (map_x=0; map_x < p_gbm->map.width; map_x++) {

                    // Set the map tile
                    if (! gbm_map_tile_set_xy(p_gbm, map_x, map_y, *(p_map_data++)))
                        return false;
            }
        }
    } else
        // Signal failure if image pointer not allocated
        return false;

    // Return success
    return true;
}