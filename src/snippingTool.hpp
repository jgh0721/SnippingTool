#ifndef SNIPPINGTOOL_HPP
#define SNIPPINGTOOL_HPP

#include <QtCore>
#include <QtWidgets>
#include "ElaWindow.h"
#include "ElaWidget.h"

// 스크린샷 영역 지정을 위한 위젯
class QSnippingWidget : public QWidget
{
    Q_OBJECT
public:
    QSnippingWidget( QPixmap Scr );

    QPixmap                             SelectedRegion();
    void                                SetDisplayAffinity( quint32 dwAffinity = 0 );

Q_SIGNALS:
    void                                sigRegionSelected();
    void                                sigUserCancelled();

protected:
    void                                paintEvent( QPaintEvent* event ) override;
    void                                keyPressEvent( QKeyEvent* event ) override;
    void                                mousePressEvent( QMouseEvent* event ) override;
    void                                mouseMoveEvent( QMouseEvent* event ) override;
    void                                mouseReleaseEvent( QMouseEvent* event ) override;

private:
    QPixmap                             screenShot_;
    QPixmap                             selectedRegion_;
    QPoint                              startPos_;
    QPoint                              endPos_;
    bool                                isSelecting_;
    quint32                             dwAffinity_;
};

class QSnippingTool : public ElaWidget
{
    Q_OBJECT
public:
    QSnippingTool( QWidget* parent = nullptr );

    QPushButton*                        GetSaveButton() const;
    QPushButton*                        GetCopyButton() const;
    void                                SetDisplayAffinity( quint32 dwAffinity = 0 );

    QPixmap                             RetrieveCaptureImage() const;

Q_SIGNALS:
    void                                sigCaptureStart( bool* IsCanceled );
    void                                sigCaptureFinished();
    // IsAccepted 가 false 이면 저장하지 않음, IsHandled 가 true 이면 별도로 메시지 상자를 표시하지 않음
    void                                sigSaveTo( const QString& FilePath, bool* IsAccepted, bool* IsHandled );
    void                                sigSaveFailed( const QString& FilePath, QImageWriter::ImageWriterError Error, const QString& ErrorText, bool* IsHandled );
    void                                sigCloseEvent( QCloseEvent* Event );

protected:
    void                                closeEvent( QCloseEvent* event ) override;
    void                                keyPressEvent( QKeyEvent* event ) override;
    void                                resizeEvent( QResizeEvent* event ) override;

private slots:
    void                                takeFullScreenshot();
    void                                takeRegionScreenshot();
    void                                takeDelayedScreenshot();
    void                                saveScreenshot();
    void                                copyToClipboard();
    void                                onRegionSelected();
    void                                takeScreenshotWithTimer();

private:

    void                                setupUi();
    void                                takeScreenshot( bool region = false, bool includeMouse = false );
    Q_INVOKABLE void                    takeScreenshotByFull( bool IncludeMouse );
    Q_INVOKABLE void                    takeScreenshotByRegion( bool IncludeMouse );

    ///////////////////////////////////////////////////////////////////////////
    /// UIs
    ///

    QLabel*                             lblCaptureImage;
    QPushButton*                        btnFullCapture;
    QPushButton*                        btnRegionCapture;
    QPushButton*                        btnTimerCapture;
    QComboBox*                          cbxTimerInterval;
    QCheckBox*                          chkIncludeCursor;
    QPushButton*                        btnSaveTo;
    QPushButton*                        btnCopyToClipboard;
    QVBoxLayout*                        mainLayout;
    QHBoxLayout*                        buttonLayout;
    QAction*                            acSaveTo;
    QAction*                            acCopyToClipboard;

    quint32                             dwAffinity;
    QPixmap                             screenshot;
    QTimer*                             delayTimer;
    QVector< QSnippingWidget* >         vecSnippingWidget;      // 모니터 수량만큼 생성
};

#endif //SNIPPINGTOOL_HPP
