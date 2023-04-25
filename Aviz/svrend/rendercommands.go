package svrend

// NOTE: this isn't used. it was for UI immediate commands

type TriangleCmd struct {
    P1, P2, P3 V2
    Rot float32
    Color uint32
}

type LineSegment struct {
    Start, End V2
    StartColor, EndColor uint32
}
