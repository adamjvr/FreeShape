#include "ui/MeasurementHud.h"

#include <cmath>
#include <sstream>
#include <string>

#include <Python.h>

#include <Base/Interpreter.h>
#include <Gui/Selection/Selection.h>

#include <QLabel>
#include <QVBoxLayout>

namespace freeshape::ui {

namespace {

std::string pythonQuote(std::string value)
{
    std::string out;
    out.reserve(value.size() + 8);
    out.push_back('\'');
    for (char c : value) {
        if (c == '\\' || c == '\'') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    out.push_back('\'');
    return out;
}

QString runSingleSelectionMeasurement(
    const Gui::SelectionSingleton::SelObj& selected
)
{
    if (selected.DocName == nullptr || selected.FeatName == nullptr) {
        return {};
    }

    const std::string doc = pythonQuote(selected.DocName);
    const std::string obj = pythonQuote(selected.FeatName);
    const std::string sub = pythonQuote(selected.SubName != nullptr ? selected.SubName : "");

    std::ostringstream script;
    script
        << "import FreeCAD as App\\n"
        << "__freeshape_measure_text = ''\\n"
        << "_doc = App.getDocument(" << doc << ")\\n"
        << "_obj = _doc.getObject(" << obj << ") if _doc else None\\n"
        << "_sub = " << sub << "\\n"
        << "if _obj is not None and hasattr(_obj, 'Shape'):\\n"
        << "    _shape = _obj.Shape\\n"
        << "    try:\\n"
        << "        if _sub.startswith('Edge'):\\n"
        << "            _i = int(_sub[4:].split('.')[0]) - 1\\n"
        << "            __freeshape_measure_text = f'Length  {_shape.Edges[_i].Length:.3f} mm'\\n"
        << "        elif _sub.startswith('Face'):\\n"
        << "            _i = int(_sub[4:].split('.')[0]) - 1\\n"
        << "            __freeshape_measure_text = f'Area  {_shape.Faces[_i].Area:.3f} mm²'\\n"
        << "        elif _sub.startswith('Vertex'):\\n"
        << "            _i = int(_sub[6:].split('.')[0]) - 1\\n"
        << "            _p = _shape.Vertexes[_i].Point\\n"
        << "            __freeshape_measure_text = f'Point  {_p.x:.3f}, {_p.y:.3f}, {_p.z:.3f} mm'\\n"
        << "        elif getattr(_shape, 'Volume', 0.0) > 0.0:\\n"
        << "            __freeshape_measure_text = f'Volume  {_shape.Volume:.3f} mm³'\\n"
        << "    except Exception:\\n"
        << "        __freeshape_measure_text = ''\\n";

    Base::PyGILStateLocker lock;
    PyObject* mainModule = PyImport_AddModule("__main__");
    if (mainModule == nullptr) {
        return {};
    }
    PyObject* globals = PyModule_GetDict(mainModule);
    if (globals == nullptr) {
        return {};
    }

    PyObject* result = PyRun_String(
        script.str().c_str(),
        Py_file_input,
        globals,
        globals
    );
    if (result == nullptr) {
        PyErr_Clear();
        return {};
    }
    Py_DECREF(result);

    PyObject* value = PyDict_GetItemString(globals, "__freeshape_measure_text");
    if (value == nullptr || !PyUnicode_Check(value)) {
        return {};
    }

    return QString::fromUtf8(PyUnicode_AsUTF8(value));
}

}  // namespace

MeasurementHud::MeasurementHud(QWidget* parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("MeasurementHud"));
    setFixedWidth(255);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 7, 10, 7);
    layout->setSpacing(2);

    title_ = new QLabel(QStringLiteral("Measure"), this);
    title_->setObjectName(QStringLiteral("MeasurementTitle"));
    detail_ = new QLabel(this);
    detail_->setWordWrap(true);

    layout->addWidget(title_);
    layout->addWidget(detail_);
    hide();
}

void MeasurementHud::refresh()
{
    const auto selection = Gui::Selection().getSelection();
    if (selection.empty()) {
        clear();
        return;
    }

    QString text;
    if (selection.size() == 1) {
        text = runSingleSelectionMeasurement(selection.front());
        if (text.isEmpty()) {
            const auto& item = selection.front();
            text = QStringLiteral("%1%2%3")
                .arg(QString::fromUtf8(item.FeatName != nullptr ? item.FeatName : ""))
                .arg(item.SubName != nullptr && *item.SubName != '\0' ? QStringLiteral(" · ") : QString())
                .arg(QString::fromUtf8(item.SubName != nullptr ? item.SubName : ""));
        }
    }
    else if (selection.size() == 2) {
        const auto& a = selection[0];
        const auto& b = selection[1];
        const double dx = static_cast<double>(a.x) - static_cast<double>(b.x);
        const double dy = static_cast<double>(a.y) - static_cast<double>(b.y);
        const double dz = static_cast<double>(a.z) - static_cast<double>(b.z);
        const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        text = QStringLiteral("Distance  %1 mm")
            .arg(distance, 0, 'f', 3);
    }
    else {
        text = QStringLiteral("%1 entities selected")
            .arg(selection.size());
    }

    detail_->setText(text);
    adjustSize();
    show();
    raise();
}

void MeasurementHud::clear()
{
    detail_->clear();
    hide();
}

QString MeasurementHud::buildMeasurementText() const
{
    return detail_->text();
}

}  // namespace freeshape::ui
