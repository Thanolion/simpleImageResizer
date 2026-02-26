// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2024-2026 thanolion

#include "FormatGuideDialog.h"

#include <QVBoxLayout>
#include <QTabWidget>
#include <QTextBrowser>
#include <QDialogButtonBox>

FormatGuideDialog::FormatGuideDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Image Format Guide");
    resize(560, 520);

    auto *layout = new QVBoxLayout(this);
    auto *tabs = new QTabWidget;

    // --- JPEG Tab ---
    auto *jpegBrowser = new QTextBrowser;
    jpegBrowser->setOpenExternalLinks(true);
    jpegBrowser->setHtml(
        "<h2>JPEG</h2>"
        "<p><b>Compression:</b> Lossy</p>"
        "<p><b>File extensions:</b> .jpg, .jpeg</p>"
        "<hr>"
        "<h3>Description</h3>"
        "<p>JPEG is the most widely used image format for photographs and complex images. "
        "It achieves excellent compression ratios by discarding visual information that is "
        "less perceptible to the human eye.</p>"
        "<h3>Best Use Cases</h3>"
        "<ul>"
        "<li>Photographs and images with smooth gradients</li>"
        "<li>Web images where file size matters</li>"
        "<li>Social media and email attachments</li>"
        "<li>Images that don't require transparency</li>"
        "</ul>"
        "<h3>Quality Settings</h3>"
        "<p>The <b>quality slider</b> (1-100) controls the compression level. Higher values "
        "preserve more detail but produce larger files. A quality of <b>80-90</b> is usually "
        "a good balance between quality and file size.</p>"
        "<p>The <b>target file size</b> option uses binary search over quality to find the "
        "best quality that fits within the specified size.</p>"
        "<h3>Limitations</h3>"
        "<ul>"
        "<li>No transparency support</li>"
        "<li>Lossy — each re-save degrades quality slightly</li>"
        "<li>Not ideal for text, line art, or sharp edges</li>"
        "</ul>"
        "<h3>Learn More</h3>"
        "<p><a href=\"https://en.wikipedia.org/wiki/JPEG\">Wikipedia: JPEG</a> · "
        "<a href=\"https://developer.mozilla.org/en-US/docs/Web/Media/Formats/Image_types#jpeg\">MDN: JPEG</a></p>"
    );
    tabs->addTab(jpegBrowser, "JPEG");

    // --- PNG Tab ---
    auto *pngBrowser = new QTextBrowser;
    pngBrowser->setOpenExternalLinks(true);
    pngBrowser->setHtml(
        "<h2>PNG</h2>"
        "<p><b>Compression:</b> Lossless</p>"
        "<p><b>File extensions:</b> .png</p>"
        "<hr>"
        "<h3>Description</h3>"
        "<p>PNG provides lossless compression, meaning no image data is lost during saving. "
        "This makes it ideal for images that require pixel-perfect reproduction, such as "
        "screenshots, diagrams, and images with transparency.</p>"
        "<h3>Best Use Cases</h3>"
        "<ul>"
        "<li>Screenshots and UI mockups</li>"
        "<li>Images with text or sharp edges</li>"
        "<li>Graphics requiring transparency</li>"
        "<li>Source images for further editing</li>"
        "</ul>"
        "<h3>Quality Settings</h3>"
        "<p><b>Note:</b> Because PNG uses lossless compression, the quality slider and "
        "target file size controls are <b>disabled</b> when PNG is selected. Every pixel "
        "is preserved exactly — there is no quality/size trade-off to configure.</p>"
        "<h3>Limitations</h3>"
        "<ul>"
        "<li>Larger file sizes than lossy formats for photographs</li>"
        "<li>Not suitable when file size is a primary concern for photos</li>"
        "</ul>"
        "<h3>Learn More</h3>"
        "<p><a href=\"https://en.wikipedia.org/wiki/PNG\">Wikipedia: PNG</a> · "
        "<a href=\"https://developer.mozilla.org/en-US/docs/Web/Media/Formats/Image_types#png\">MDN: PNG</a></p>"
    );
    tabs->addTab(pngBrowser, "PNG");

    // --- WebP Tab ---
    auto *webpBrowser = new QTextBrowser;
    webpBrowser->setOpenExternalLinks(true);
    webpBrowser->setHtml(
        "<h2>WebP</h2>"
        "<p><b>Compression:</b> Lossy (with lossless option)</p>"
        "<p><b>File extensions:</b> .webp</p>"
        "<hr>"
        "<h3>Description</h3>"
        "<p>WebP is a modern image format developed by Google that provides superior "
        "compression compared to JPEG while supporting transparency. It is widely "
        "supported in modern web browsers.</p>"
        "<h3>Best Use Cases</h3>"
        "<ul>"
        "<li>Web images — smaller than JPEG with comparable quality</li>"
        "<li>Images needing both compression and transparency</li>"
        "<li>Replacing JPEG/PNG on websites for faster loading</li>"
        "</ul>"
        "<h3>Quality Settings</h3>"
        "<p>The <b>quality slider</b> (1-100) works similarly to JPEG. Higher values "
        "mean better quality and larger files. WebP typically produces <b>25-35% smaller</b> "
        "files than JPEG at equivalent visual quality.</p>"
        "<p>The <b>target file size</b> option is supported.</p>"
        "<h3>Limitations</h3>"
        "<ul>"
        "<li>Not universally supported by older image editors</li>"
        "<li>Some social media platforms may not accept WebP uploads</li>"
        "</ul>"
        "<h3>Learn More</h3>"
        "<p><a href=\"https://en.wikipedia.org/wiki/WebP\">Wikipedia: WebP</a> · "
        "<a href=\"https://developers.google.com/speed/webp\">Google: WebP</a></p>"
    );
    tabs->addTab(webpBrowser, "WebP");

    // --- AVIF Tab ---
    auto *avifBrowser = new QTextBrowser;
    avifBrowser->setOpenExternalLinks(true);
    avifBrowser->setHtml(
        "<h2>AVIF</h2>"
        "<p><b>Compression:</b> Lossy (with lossless option)</p>"
        "<p><b>File extensions:</b> .avif</p>"
        "<hr>"
        "<h3>Description</h3>"
        "<p>AVIF is a next-generation image format based on the AV1 video codec. It offers "
        "superior compression — typically <b>25-35% smaller than WebP</b> and up to "
        "<b>50% smaller than JPEG</b> at equivalent visual quality. AVIF is rapidly "
        "gaining browser and application support.</p>"
        "<h3>Best Use Cases</h3>"
        "<ul>"
        "<li>Maximum compression for web images</li>"
        "<li>High dynamic range (HDR) images</li>"
        "<li>Images requiring both small size and transparency</li>"
        "<li>Photography where every byte counts</li>"
        "</ul>"
        "<h3>Quality Settings</h3>"
        "<p>The <b>quality slider</b> (1-100) controls compression. AVIF achieves "
        "visually lossless results at lower quality values than JPEG or WebP, meaning "
        "a quality of <b>60-75</b> often looks as good as JPEG at 85-90.</p>"
        "<p>The <b>target file size</b> option is supported.</p>"
        "<h3>Limitations</h3>"
        "<ul>"
        "<li>Encoding is slower than JPEG or WebP (AV1-based)</li>"
        "<li>Not yet supported by all image editors and platforms</li>"
        "<li>Older browsers (pre-2022) may lack support</li>"
        "</ul>"
        "<h3>Learn More</h3>"
        "<p><a href=\"https://en.wikipedia.org/wiki/AVIF\">Wikipedia: AVIF</a> · "
        "<a href=\"https://aomediacodec.github.io/av1-avif/\">AVIF Specification</a></p>"
    );
    tabs->addTab(avifBrowser, "AVIF");

    layout->addWidget(tabs);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);
}
