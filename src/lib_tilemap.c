//
// lib_tilemap.c
//
#include <stdio.h>
#include <string.h>


#include "lib_tilemap.h"
#include "tilemap_tiles.h"
#include "tilemap_io.h"

#include "hash.h"

// Globals
tile_map_data tile_map;
tile_set_data tile_set;



// TODO: support configurable tile size
int tilemap_initialize(image_data * p_src_img, uint16_t search_mask) {

    // Tile Map
    tile_map.map_width   = p_src_img->width;
    tile_map.map_height  = p_src_img->height;

    tile_map.tile_width  = TILE_WIDTH_DEFAULT;
    tile_map.tile_height = TILE_HEIGHT_DEFAULT;

    tile_map.width_in_tiles  = tile_map.map_width  / tile_map.tile_width;
    tile_map.height_in_tiles = tile_map.map_height / tile_map.tile_height;

    tile_map.search_mask = search_mask;

    // Max space required to store Tile Map is
    // width x height in tiles (if every map tile is unique)
    tile_map.size = (tile_map.width_in_tiles * tile_map.height_in_tiles);

    // if TILES_MAX_DEFAULT > 255, tile_id_list must be larger than uint8_t -> uint32_t
    //tile_map.tile_id_list = malloc(tile_map.size * sizeof(uint32_t));
    tile_map.tile_id_list = malloc(tile_map.size);
    if (!tile_map.tile_id_list)
            return(false);

    tile_map.tile_attribs_list = malloc(tile_map.size * sizeof(uint16_t));
    if (!tile_map.tile_attribs_list)
            return(false);


    // Tile Set
    tile_set.tile_bytes_per_pixel = p_src_img->bytes_per_pixel;
    tile_set.tile_width  = TILE_WIDTH_DEFAULT;
    tile_set.tile_height = TILE_HEIGHT_DEFAULT;
    tile_set.tile_size   = tile_set.tile_width * tile_set.tile_height * tile_set.tile_bytes_per_pixel;
    tile_set.tile_count  = 0;

    return (true);
}



unsigned char tilemap_export_process(image_data * p_src_img, int check_flip) {

    uint16_t search_mask;

    if (check_flip) search_mask = TILE_FLIP_BITS_XY;
        else        search_mask = TILE_FLIP_BITS_NONE;

    if ( check_dimensions_valid(p_src_img) ) {
        if (!tilemap_initialize(p_src_img, search_mask)) { // Success, prep for processing
            return (false); // Signal failure and exit
        }
    }
    else {
        return (false); // Signal failure and exit
    }

    if ( ! process_tiles(p_src_img) )
        return (false); // Signal failure and exit

    return (true);
}



unsigned char process_tiles(image_data * p_src_img) {

    int             img_x, img_y;

    tile_data       tile, flip_tiles[2];
    tile_map_entry  map_entry;
    uint32_t        img_buf_offset;
    int32_t         map_slot;

    map_slot = 0;

    // Use pre-initialized values from tilemap_initialize()
    tile_initialize(&tile, &tile_map, &tile_set);
    tile_initialize(&flip_tiles[0], &tile_map, &tile_set);
    tile_initialize(&flip_tiles[1], &tile_map, &tile_set);

    if (tile.p_img_raw) {

        // Iterate over the map, top -> bottom, left -> right
        img_buf_offset = 0;

        for (img_y = 0; img_y < tile_map.map_height; img_y += tile_map.tile_height) {
            for (img_x = 0; img_x < tile_map.map_width; img_x += tile_map.tile_width) {

                // Set buffer offset to upper left of current tile
                img_buf_offset = (img_x + (img_y * tile_map.map_width)) * p_src_img->bytes_per_pixel;

                tile_copy_tile_from_image(p_src_img,
                                          &tile,
                                          img_buf_offset);

                // TODO! Don't hash transparent pixels? Have to overwrite second byte?
                tile.hash[0] = MurmurHash2( tile.p_img_raw, tile.raw_size_bytes, 0xF0A5); // len is u8count, 0xF0A5 is seed

                map_entry = tile_find_match(tile.hash[0], &tile_set, tile_map.search_mask);


                // Tile not found, create a new entry
                if (map_entry.id == TILE_ID_NOT_FOUND) {

                    // Calculate remaining hash flip variations
                    // (only for tiles that get registered)
                    if (tile_map.search_mask)
                        tile_calc_alternate_hashes(&tile, flip_tiles);

                    map_entry = tile_register_new(&tile, &tile_set);

                    if (map_entry.id == TILE_ID_OUT_OF_SPACE) {
                        tile_free(&tile);
                        tile_free(&flip_tiles[0]);
                        tile_free(&flip_tiles[1]);
                        tilemap_free_resources();

                        printf("Tilemap: Process: FAIL -> Too Many Tiles\n");
                        return (false); // Ran out of tile space, exit
                    }
                }

                tile_map.tile_id_list[map_slot]      = map_entry.id;
                tile_map.tile_attribs_list[map_slot] = map_entry.attribs;

                map_slot++;
            }
        }

    } else { // else if (tile.p_img_raw) {
        tilemap_free_resources();
        return (false); // Failed to allocate buffer, exit
    }

    // Free resources
    tile_free(&tile);
    tile_free(&flip_tiles[0]);
    tile_free(&flip_tiles[1]);

    printf("Total Tiles=%d\n", tile_set.tile_count);
}


static int32_t check_dimensions_valid(image_data * p_src_img) {

    // Image dimensions must be exact multiples of tile size
    if ( ((p_src_img->width % TILE_WIDTH_DEFAULT) != 0) ||
         ((p_src_img->height % TILE_HEIGHT_DEFAULT ) != 0))
        return false; // Fail
    else
        return true;  // Success
}




void tilemap_free_tile_set(void) {
        int c;

    // Free all the tile set data
    for (c = 0; c < tile_set.tile_count; c++) {

        if (tile_set.tiles[c].p_img_encoded)
            free(tile_set.tiles[c].p_img_encoded);
        tile_set.tiles[c].p_img_encoded = NULL;

        if (tile_set.tiles[c].p_img_raw)
            free(tile_set.tiles[c].p_img_raw);
        tile_set.tiles[c].p_img_raw = NULL;
    }

    tile_set.tile_count  = 0;
}

void tilemap_free_resources(void) {

    tilemap_free_tile_set();

    // Free tile map data
    if (tile_map.tile_id_list) {
        free(tile_map.tile_id_list);
        tile_map.tile_id_list = NULL;
    }

    if (tile_map.tile_attribs_list) {
        free(tile_map.tile_attribs_list);
        tile_map.tile_id_list = NULL;
    }

}



int32_t tilemap_save(const int8_t * filename, uint32_t export_format) {

    return( tilemap_export(filename,
                           export_format,
                           &tile_map,
                           &tile_set) );
}



tile_map_data * tilemap_get_map(void) {
    return (&tile_map);
}



tile_set_data * tilemap_get_tile_set(void) {
    return (&tile_set);
}



// TODO: Consider moving this to a different location
//
// Returns an image which is a composite of all the
// tiles in a tile map, in order.
int32_t tilemap_get_image_of_deduped_tile_set(image_data * p_img) {

    uint32_t c;
    uint32_t img_offset;

    // Set up image to store deduplicated tile set
    p_img->width  = tile_map.tile_width;
    p_img->height = tile_map.tile_height * tile_set.tile_count;
    p_img->size   = tile_set.tile_size   * tile_set.tile_count;
    p_img->bytes_per_pixel = tile_set.tile_bytes_per_pixel;

printf("== COPY TILES INTO COMPOSITE BUF %d x %d, total size=%d\n", p_img->width, p_img->height, p_img->size);
    // Allocate a buffer for the image
    p_img->p_img_data = malloc(p_img->size);

    if (p_img->p_img_data) {

        img_offset = 0;

        for (c = 0; c < tile_set.tile_count; c++) {

            if (tile_set.tiles[c].p_img_raw) {
                // Copy from the tile's raw image buffer (indexed)
                // into the composite image

tile_print_buffer_raw(tile_set.tiles[c]); // TODO: remove

                memcpy(p_img->p_img_data + img_offset,
                       tile_set.tiles[c].p_img_raw,
                       tile_set.tile_size);
            }
            else
                return false;

            img_offset += tile_set.tile_size;
        }
    }
    else
        return false;

    printf("== TILEMAP -> IMG COPIED BUFF\n");

    // Iterate over each tile, top -> bottom, left -> right
    for (c = 0; c < p_img->size; c++) {
        printf(" %2x", *(p_img->p_img_data + c));
        if ((c % 8) ==0) printf("\n");
    }
    printf(" \n");

    return true;
}
