#ifndef GENRES_H_
#define GENRES_H_


/**
 *  \struct genresTiles
 *      GenRes tile structure
 */
struct genresTiles
{
	u16 *pal; 		//pointer to pal data
	u32 *tiles;		//pointer to tiles data
	u16 width;		//width in tiles
	u16 height;		//height in tiles
	u16 compressedSize; //0 in this demo, more coming soon
};


/**
 *  \struct genresSprites
 *      GenRes Sprites definition structure.<br/>
 *      - pal: pointer to pal data<br/>
 *      - sprites: pointer to sprites data<br/>
 *      - count: sprite number<br/>
 *      - width: width of each sprite in pixels (not tiles!)<br/>
 *      - height: height of each sprite in pixels (not tiles!)<br/>
 *      - size: since we use width/height in pixel, useful info on sprite size<br/>
 */
struct genresSprites
{
	u16 *pal; 		//pointer to pal data
	u32 **sprites;		//pointer to sprites data
	u16 count;		//nb sprites
	u16 width;		//width of each sprite in pixels
	u16 height;		//height of each sprite in pixels
	u16 size; 		//since we use width/height in pixel, useful info on sprite size
	//TODO : size is not SGDK compliant, you need to use size>>8
};


//ANIMATION
struct animSpriteInfo{
	u16	idx;
	u8	x,y; // upon frame upper left
};

struct animFrame {
	u16	nbSprites;
	struct 	animSpriteInfo	*sprites;

	u8	timer;
	u8	nextFrameIdx;
};

struct genresAnimation {
	u16	*pal;
	u16	width, height;

	//frame data
	u16	nbFrames;
	struct	animFrame		**frames;

	//sprite data
	u8 	sprite_width;
	u8 	sprite_height;
	u16 nbTiles;
	u16	*sprite_data;
};

//PAL
//typedef u16	genresPal[16];
struct genresPal {
	u16 color[16];
};


#endif /* GENRES_H_ */
