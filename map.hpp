#ifndef MAP_HPP
#define MAP_HPP

#include <vector>
#include <stdexcept>
#include <limits>

#define HEXAGON_VERTEX_COUNT 6

using opt_t = unsigned;

struct Position {
    int i, j;
};

struct Point {
    float x, y;
};

enum class HexType {
    regular,
    wall,
    cat
};

enum class Status {
    playing,
    win,
    fail
};

enum Option {
    selected = 1 << 0,
    final = 1 << 1,
    way_higlight = 1 << 2,
    prohibited = 1 << 3,
};

struct HexTile {
    Point v[HEXAGON_VERTEX_COUNT];
    HexType type;
    opt_t opt;
};

struct HexTile_Info {
    HexTile* tile;
    Position p;
};

class Map {

    //      v4
    //      /\
    //  v3 /  \ v5
    //    |    |
    //    |    |
    //  v2 \  / v6
    //      \/
    //      v1

    using Tiles = std::vector<std::vector<HexTile>>;
    using Way = std::vector<HexTile_Info>;

    Tiles m_tiles; // NDC Coordinates
    HexTile_Info m_cat = {nullptr, Position{0, 0}};
    Status m_status = Status::playing;

    bool inHexagon(Point pt, const Point *v);
    bool inTriangle(Point pt, const Point *v);
    bool contains(const Way& way, const HexTile* tile);

    Way findShortestWay(Position p, Way way = Way(), std::vector<HexTile_Info> prohibit = std::vector<HexTile_Info>(), Way::size_type max_length = std::numeric_limits<Way::size_type>::max());
    bool turn(HexTile& tile);
public:
    Map();

    void clickOn(Point pt);
    void enter(Point pt);

    bool setWall(Position p);
    void select(Position p);
    void deselect(Position p);
    bool within(Position p) const;
    Status status() const;

    Tiles::size_type height() const { return m_tiles.size(); }
    Tiles::size_type width() const { return m_tiles[0].size(); }

    HexTile& at(Tiles::size_type i, Tiles::size_type j) {
        if (i >= height() or j >= width())
            throw std::out_of_range("Hexagon is not exist.");
        return m_tiles[i][j];
    }

    HexTile& at(Position p) {
        return this->at(Tiles::size_type(p.i), Tiles::size_type(p.j));
    }

};
#endif // MAP_HPP
