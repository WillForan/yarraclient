#ifndef ORT_RETURNONFOCUS_H
#define ORT_RETURNONFOCUS_H

#include <QtWidgets>

// Helper class to patch QLineEdit with InputMask to jump to the
// first character when focusing into the line edit control.
//
// Solution taken from:
// http://stackoverflow.com/questions/22532607/qlineedit-set-cursor-location-to-beginning-on-focus

class ortReturnOnFocus : public QObject
{
   Q_OBJECT

   // Catches FocusIn events on the target line edit, and appends a call
   // to resetCursor at the end of the event queue.
   bool eventFilter(QObject * obj, QEvent * ev)
   {
      QLineEdit * w = qobject_cast<QLineEdit*>(obj);

      // w is nullptr if the object isn't a QLineEdit
      if (w && ev->type()==QEvent::FocusIn)
      {
         QMetaObject::invokeMethod(this, "resetCursor",
                                   Qt::QueuedConnection, Q_ARG(QWidget*, w));
      }

      // A base QObject is free to be an event filter itself
      return QObject::eventFilter(obj, ev);
   }

   // Resets the cursor position of a given widget. Widget must be a line edit.
   Q_INVOKABLE void resetCursor(QWidget * w)
   {
      static_cast<QLineEdit*>(w)->setCursorPosition(0);
   }

public:
   ortReturnOnFocus(QObject * parent = 0)
       : QObject(parent)
   {
   }

   // Installs the reset functionality on a given line edit
   void installOn(QLineEdit * ed)
   {
       ed->installEventFilter(this);
   }

};



#endif // ORT_RETURNONFOCUS_H
