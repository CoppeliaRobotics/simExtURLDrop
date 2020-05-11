#ifndef EVENTFILTER_H
#define EVENTFILTER_H

#include <QObject>
#include <QEvent>
#include <QWidget>
#include <QProgressDialog>

class EventFilter : public QObject
{
    Q_OBJECT
public:
	explicit EventFilter(QWidget *mainWin, QObject *parent = 0);
	virtual ~EventFilter();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QWidget *mainWindow;
    QProgressDialog *progressDialog;
};

#endif // EVENTFILTER_H

