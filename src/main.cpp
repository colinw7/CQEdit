#include <CQEditTest.h>
#include <CQEdit.h>
#include <CQApp.h>

int
main(int argc, char **argv)
{
  CQApp app(argc, argv);

  std::vector<std::string> filenames;

  std::string replay;

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      if (strcmp(&argv[i][1], "replay") == 0) {
        ++i;

        if (i < argc)
          replay = argv[i];
      }
    }
    else
      filenames.push_back(argv[i]);
  }

  //------

  CQEdit::init();

  CQEditTest *edit = new CQEditTest;

  if (! filenames.empty()) {
    uint num_files = filenames.size();

    for (uint i = 0; i < num_files; ++i)
      edit->addFile(filenames[i]);
  }
  else
    edit->addFile("");

  edit->resize(1200, 1600);

  edit->show();

  if (replay != "")
    edit->getEdit()->getFile()->replayFile(replay);

  return app.exec();
}
