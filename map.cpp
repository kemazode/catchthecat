#include <random>
#include <iostream>
#include <thread>

#include "util.hpp"
#include "map.hpp"

#define INDENT_FROM_BORDER 3

const int map_width = 10;
const int map_height = 10;
const int walls_count = int(map_width*map_height * 0.1); // 10% of all map

float _sign(Point p1, Point p2, Point p3)
{
    return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

bool Map::inTriangle(Point pt, const Point *v) {
    float d1, d2, d3;
    bool has_neg, has_pos;

    d1 = _sign(pt, v[0], v[1]);
    d2 = _sign(pt, v[1], v[2]);
    d3 = _sign(pt, v[2], v[0]);

    has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(has_neg && has_pos);
}

bool Map::inHexagon(Point pt, const Point *v) {
    const Point t[4][3] = {{v[0], v[1], v[5]},
                           {v[2], v[3], v[4]},
                           {v[1], v[2], v[4]},
                           {v[1], v[4], v[5]}};

    return inTriangle(pt, t[0]) or
            inTriangle(pt, t[1]) or
            inTriangle(pt, t[2]) or
            inTriangle(pt, t[3]);
}

Map::Map() {


    m_tiles.resize(map_height);
    for (int i = 0; i < map_height; ++i)
        for (int j = 0; j < map_width; ++j) {
            m_tiles[i].resize(map_width);
            m_tiles[i][j].type = HexType::regular;
        }

    // Set finals
    for (int i = 0; i < map_height; ++i) {
        m_tiles[i][0].opt |= static_cast<opt_t>(Option::final);
        m_tiles[i][map_width - 1].opt |= Option::final;
    }

    for (int j = 0; j < map_width; ++j) {
        m_tiles[0][j].opt |= Option::final;
        m_tiles[map_height - 1][j].opt |= Option::final;
    }

    // Set walls
    for (int i = 0; i < walls_count; ++i)
        at( Position{std::rand() % map_height, std::rand() % map_width} ).type = HexType::wall;

    // Set cat
    Position cat = {
        INDENT_FROM_BORDER + std::rand() % (map_height - INDENT_FROM_BORDER * 2),
        INDENT_FROM_BORDER + std::rand() % (map_width - INDENT_FROM_BORDER * 2)
    };

    m_cat.p = cat;
    m_cat.tile = &at(cat);
    m_cat.tile->type = HexType::cat;
}


void Map::clickOn(Point pt) {
    HexTile *p = nullptr;
    for (auto &i : m_tiles)
        for (auto &j : i)
            if (inHexagon(pt, j.v)) {
                p = &j;
                break;
            }

    if (not p)
        return;

    turn(*p);
}

void Map::enter(Point pt) {
    for (auto &i : m_tiles)
    for (auto &j : i)
        if (inHexagon(pt, j.v)) {
            j.opt |= Option::selected;
        } else
            j.opt &= ~Option::selected;
}

bool Map::turn(HexTile &tile) {

    if (tile.type != HexType::regular or m_status != Status::playing)
        return false;

    tile.type = HexType::wall;

#ifdef PATH_HIGHLIGHT
    for (auto &i : m_tiles)
        for (auto &j : i)
            j.opt &= ~Option::way_higlight;
#endif

    Way way = findShortestWay(m_cat.p);

    if (not way.empty() and way.front().tile->opt & Option::final) {
        std::cout << "You have lose!" << std::endl;
        m_status = Status::fail;
    }
    else if (way.empty()) {
        std::cout << "You have won!" << std::endl;
        m_status = Status::win;
    }

    if (not way.empty()) {
#ifdef PATH_HIGHLIGHT
        for (auto i = way.begin(); i < way.end(); ++i)
            i->tile->opt |= Option::way_higlight;
#endif

        m_cat.tile->type = HexType::regular;

        m_cat = way.front();
        m_cat.tile->type = HexType::cat;
    }

    return true;
};

Map::Way Map::findShortestWay(Position p, Way way, std::vector<HexTile_Info> prohibit, Way::size_type max_length) {
    if (this->at(p).opt & Option::final) {
        return way;
    }


    if (way.size() >= max_length){
        return Way();
    }

    std::vector<Way> possible_ways;

    Position neighbors_positions[6] = {
        {p.i+1, p.j - (p.i&1)}, {p.i+1, p.j+1 - (p.i&1)},
        {p.i, p.j-1}, {p.i, p.j+1},
        {p.i-1, p.j - (p.i&1)}, {p.i-1, p.j+1 - (p.i&1)}
    };

    std::vector<HexTile_Info> neighbors;
    for (int i = 0; i < 6; ++i)
        if (within(neighbors_positions[i]) and
            at(neighbors_positions[i]).type != HexType::wall and
            not contains(prohibit, &at(neighbors_positions[i])))
            neighbors.push_back(HexTile_Info{
                                    &at(neighbors_positions[i]),
                                    neighbors_positions[i]
                                });

    if (prohibit.empty())
        prohibit.push_back(HexTile_Info{&at(p), p});

    prohibit.insert(prohibit.end(), neighbors.begin(), neighbors.end());

    // Call searching recursively
    for (auto &i : neighbors) {
        Way new_way(way);
        new_way.push_back(i);

        if (i.p.i == 3 && i.p.j == 8)
            std::cout << "";

        new_way = findShortestWay(i.p, new_way, prohibit, max_length);

        if (not new_way.empty()) {
            max_length = std::min(max_length, new_way.size());
            possible_ways.push_back(new_way);
        }
    }

    // If no possible ways than check given one and if that is not final return empty way
    if (possible_ways.empty()) {
        if (not way.empty() and way.back().tile->opt & Option::final) {
            return way;
        }
        else {
            return Way();
        }
    }

    // Find the minimal way from possible_ways
    std::vector<Way>::size_type shortest = 0;
    auto shortest_size = possible_ways.front().size();
    for (std::vector<Way>::size_type i = 0; i < possible_ways.size(); ++i)
        if (possible_ways[i].size() < shortest_size) {
            shortest_size = possible_ways[i].size();
            shortest = i;
        }

    // Return minimal way
    return possible_ways[shortest];
}

void Map::select(Position p)
{
    at(p).opt |= Option::selected;
}

void Map::deselect(Position p)
{
    at(p).opt &= ~Option::selected;
}

bool Map::setWall(Position p)
{
    return turn(at(p));
}

bool Map::within(Position p) const {
    return p.i >= 0 and
           p.j >= 0 and
           Tiles::size_type(p.i) < height() and
           Tiles::size_type(p.j) < width();
}

bool Map::contains(const Way& way, const HexTile* tile)
{
    for (auto& i : way)
        if (i.tile == tile)
            return true;
    return false;
}

Status Map::status() const {
    return m_status;
}
