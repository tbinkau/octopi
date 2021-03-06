/*
  Source code extracted from:
  http://www.jakepetroules.com/2011/07/10/creating-a-windows-explorer-style-search-box-in-qt

  Written by Jake Petroules
  Adapted to suit QTGZManager
*/

#include "searchlineedit.h"
#include "strconstants.h"
#include "mainwindow.h"

#include <QApplication>
#include <QToolButton>
#include <QStyle>

SearchLineEdit::SearchLineEdit(QWidget *parent) :
  QLineEdit(parent){

  // Create the search button and set its icon, cursor, and stylesheet
  this->mSearchButton = new QToolButton(this);

  // Increase button size a bit for kde
  if (WMHelper::isKDERunning())
    this->mSearchButton->setFixedSize(20, 20);
  else
    this->mSearchButton->setFixedSize(18, 18);

  this->mSearchButton->setCursor(Qt::ArrowCursor);
  this->mSearchButton->setStyleSheet(this->buttonStyleSheetForCurrentState());

  // Update the search button when the text changes
  QObject::connect(this, SIGNAL(textChanged(QString)), SLOT(updateSearchButton(QString)));

  // Some stylesheet and size corrections for the text box
#if QT_VERSION >= 0x040700
  this->setPlaceholderText(StrConstants::getFind());
#else
  this->setToolTip(StrConstants::getFind());
#endif

  this->setStyleSheet(this->styleSheetForCurrentState());
}

void SearchLineEdit::resizeEvent(QResizeEvent *event)
{
  Q_UNUSED(event);
  this->mSearchButton->move(5, (this->rect().height() - this->mSearchButton->height()) / 2);
}

void SearchLineEdit::updateSearchButton(const QString &text)
{
  if (!text.isEmpty()){
    // We have some text in the box - set the button to clear the text
    QObject::connect(this->mSearchButton, SIGNAL(clicked()), SLOT(clear()));
  }
  else{
    // The text box is empty - make the icon do nothing when clicked
    QObject::disconnect(this->mSearchButton, SIGNAL(clicked()), this, SLOT(clear()));
  }

  //this->setStyleSheet(this->styleSheetForCurrentState());
  this->mSearchButton->setStyleSheet(this->buttonStyleSheetForCurrentState());
}

QString SearchLineEdit::styleSheetForCurrentState()
{
  int frameWidth = 1;
  QString style;
  style += "QLineEdit {";

  if (this->text().isEmpty() && (UnixCommand::getLinuxDistro() != ectn_CHAKRA))
  {
    style += "font-family: 'MS Sans Serif';";
    style += "font-style: italic;";
  }
  else
  {
    QFont font(QApplication::font());
    font.setItalic(true);
    setFont(font);
  }

  if(UnixCommand::getLinuxDistro() != ectn_CHAKRA)
  {
    style += "padding-left: 20px;";
    style += QString("padding-right: %1px;").arg(this->mSearchButton->sizeHint().width() + frameWidth + 1);
    style += "border-width: 3px;";
    style += "border-image: url(:/resources/images/esf-border.png) 3 3 3 3 stretch;";
    style += "background-color: rgba(255, 255, 255, 255);"; //204);";
    style += "color: black;}";
  }
  else
  {
    style += "padding-left: 20px;}";
    setPalette(QApplication::palette());
  }

  return style;
}

void SearchLineEdit::setFoundStyle(){
  QString style;
  style += "QLineEdit {";

  if (UnixCommand::getLinuxDistro() != ectn_CHAKRA)
  {
    style += "font-family: 'MS Sans Serif';";
    style += "font-style: italic;";
    style += "padding-left: 20px;";
    style += QString("padding-right: %1px;").arg(this->mSearchButton->sizeHint().width() + 2);
    style += "border-width: 3px;";
    style += "border-image: url(:/resources/images/esf-border.png) 3 3 3 3 stretch;";
    style += "color: black; ";
    style += "background-color: rgb(255, 255, 200);";
    style += "border-color: rgb(206, 204, 197);}";
    setStyleSheet(style);
  }
  else
  // setPalette() must be called after setStyleSheet()
  {
    style += "padding-left: 20px;}";
    setStyleSheet(style);

    QPalette palette(QApplication::palette());
    palette.setColor(QPalette::Base, QColor(255, 255, 200));
    palette.setColor(QPalette::Text, Qt::darkGray); // give more contrast to text
    setPalette(palette);
  }
}

void SearchLineEdit::setNotFoundStyle(){
  QString style;
  style += "QLineEdit {";

  if (UnixCommand::getLinuxDistro() != ectn_CHAKRA)
  {
    style += "font-family: 'MS Sans Serif';";
    style += "font-style: italic;";
    style += "padding-left: 20px;";
    style += QString("padding-right: %1px;").arg(this->mSearchButton->sizeHint().width() + 2);
    style += "border-width: 3px;";
    style += "border-image: url(:/resources/images/esf-border.png) 3 3 3 3 stretch;";
    style += "color: white; ";
    style += "background-color: lightgray;"; //rgb(255, 108, 108); //palette(mid);"; //rgb(207, 135, 142);";
    style += "border-color: rgb(206, 204, 197);}";
    setStyleSheet(style);
  }
  // setPalette() must be called after setStyleSheet()
  else
  //if(UnixCommand::getLinuxDistro() == ectn_CHAKRA)
  {
    style += "padding-left: 20px;}";
    setStyleSheet(style);

    QPalette palette(QApplication::palette());
    palette.setColor(QPalette::Base, Qt::lightGray);
    palette.setColor(QPalette::Text, Qt::white);
    setPalette(palette);
  }
}

QString SearchLineEdit::buttonStyleSheetForCurrentState() const
{
  // When using KDE avoid stylesheet customization
  if (WMHelper::isKDERunning() && UnixCommand::getLinuxDistro() != ectn_KAOS) {
    this->text().isEmpty() ? this->mSearchButton->setIcon(IconHelper::getIconSearch())
                           : this->mSearchButton->setIcon(IconHelper::getIconClear());
    this->mSearchButton->setAutoRaise(true);
    return QString();
  }

  QString style;
  style += "QToolButton {";
  style += "border: none; margin: 0; padding: 0;";
  style += QString("background-image: url(:/resources/images/esf-%1.png);").arg(this->text().isEmpty() ? "search" : "clear");
  style += "}";

  if (!this->text().isEmpty())
  {
    style += "QToolButton:pressed { background-image: url(:/resources/images/esf-clear-active.png); }";
    this->mSearchButton->setToolTip(StrConstants::getClear());
  }
  else this->mSearchButton->setToolTip("");

  return style;
}
