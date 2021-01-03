#include <CQVi.h>

#include <QApplication>

int
main(int argc, char **argv)
{
  QApplication app(argc, argv);

  std::string filename;

  auto *edit = new CQVi;

  if (argc > 1)
    edit->loadFile(argv[1]);

  edit->show();

  edit->resize(edit->sizeHint());

  return app.exec();
}
