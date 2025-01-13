class Vec2
{
private:
    float m_x;
    float m_y;
public:
    const float x();
    const float y();
    void x(float x);
    void y(float y);
    Vec2();
    Vec2(float x, float y);
    ~Vec2();
};
