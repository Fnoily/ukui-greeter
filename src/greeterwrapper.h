#ifndef GREETERWRAPPER_H
#define GREETERWRAPPER_H

#include <QLightDM/Greeter>

class GreeterWrapper : public QLightDM::Greeter
{
    Q_OBJECT
public:
    explicit GreeterWrapper(QObject *parent=0);
    void setLang(const QString&);
    QString lang();

private:
    QString m_language;
};

#endif // GREETERWRAPPER_H