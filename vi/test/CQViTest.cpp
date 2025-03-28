#include <CQVi.h>
#include <CQApp.h>

int
main(int argc, char **argv)
{
  CQApp app(argc, argv);

  std::string filename;

  auto *edit = new CQVi::Widget;

  edit->init();

  if (argc > 1)
    edit->loadFile(argv[1]);

  edit->show();

  edit->resize(edit->sizeHint());

  return app.exec();
}
