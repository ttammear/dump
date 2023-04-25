package svrend

type RectPackState struct {
    size IV2
    full bool

    x int32
    y int32
    maxy int32
}

func (rp *RectPackState) PlaceRectangle(s IV2) IV2 {
    if(rp.full) {
        return IV2{-100, -100}
    }
    if(s.X > rp.size.X || s.Y > rp.size.Y) {
        panic("Packed rect can't be larger than atlas!")
    }
    if(rp.x + s.X > rp.size.X) { // new shelf
        rp.x = 0
        rp.y += rp.maxy
        rp.maxy = 0
    }
    if(s.Y > rp.maxy) {
        rp.maxy = s.Y+1
        if(rp.y + rp.maxy > rp.size.Y) {
            // TODO: new page
            panic("Font atlas full! Please handle this case!")
            rp.full = true
            return IV2{-100, -100}
        }
    }
    ret := IV2{rp.x , rp.y}
    rp.x += s.X+1
    return ret
}

func InitRectPackState(rp *RectPackState, atlasWidth, atlasHeight int32) {
    rp.size = IV2{atlasWidth, atlasHeight}
}
