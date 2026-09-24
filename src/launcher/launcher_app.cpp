#include <wx/wx.h>

#include "launcher_frame.hpp"

namespace launcher
{

class LauncherApp: public wxApp
{
  public:
   bool OnInit() override
   {
      if (!wxApp::OnInit())
      {
         return false;
      }
      auto* frame = new LauncherFrame();
      frame->Show();
      return true;
   }
};

}  // namespace launcher

wxIMPLEMENT_APP(launcher::LauncherApp);
