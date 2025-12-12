#pragma once

namespace frik
{
    class SelfieHandler
    {
    public:
        void onFrameUpdate() const;

    private:
        void basicSelfie() const;
        void testSelfie();
        void enterSelfieMode();
        void exitSelfieMode() const;
        static void experimental();

        bool _selfieActive = false;
        RE::NiPoint3 _playerStartPosition;
        RE::NiPoint3 _forwardDir;
        RE::NiPoint3 _rootWorldPos;
    };
}
