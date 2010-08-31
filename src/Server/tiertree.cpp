#include <QtXml>
#include "tiertree.h"
#include "tier.h"

TierCategory::~TierCategory()
{
    clear();
}

void TierCategory::cleanCategories()
{
    foreach(TierCategory *c, subCategories) {
        c->cleanCategories();
    }

    foreach(TierCategory *c, subCategories) {
        if (c->subCategories.empty() && c->subLeafs.empty()) {
            subCategories.removeOne(c);
        }
    }
}

void TierCategory::serialize(QDataStream &stream, int level)
{
    if (!root) {
        stream << uchar(level) << name;
    }

    level += 1;

    foreach(TierCategory *c, subCategories) {
        c->serialize(stream, level);
    }

    foreach(Tier *t, subLeafs) {
        stream << uchar (level) << t->name();
    }
}

void TierCategory::kill(Tier *t) {
    subLeafs.removeAll(t);
    delete t;
}

void TierCategory::clear()
{
    foreach(TierCategory *c, subCategories) {
        c->clear();
        delete c;
    }
    foreach(Tier *t, subLeafs) {
        delete t;
    }
    subLeafs.clear();
    subCategories.clear();
    name.clear();
}

void TierCategory::clearWithoutDeleting()
{
    subLeafs.clear();
    subCategories.clear();
    name.clear();
}

void TierCategory::loadFromXml(const QDomElement &elem, TierMachine *boss, bool root)
{
    clear();

    if (!root)
        name = elem.attribute("name");

    this->root = root;

    QDomElement n = elem.firstChildElement();
    while(!n.isNull()) {
        if (n.tagName() == "category") {
            TierCategory *c = new TierCategory();
            c->loadFromXml(n, boss);
            subCategories.push_back(c);
        } else if (n.tagName() == "tier") {
            Tier *t = new Tier(boss, this);
            t->loadFromXml(n);
            subLeafs.push_back(t);
        }
        n = n.nextSiblingElement();
    }
}

TierCategory *TierCategory::dataClone() const
{
    TierCategory *c = new TierCategory();
    c->name = name;
    c->root = root;

    foreach(TierCategory *tc, subCategories) {
        c->subCategories.push_back(tc->dataClone());
    }

    foreach(Tier *t, subLeafs) {
        c->subLeafs.push_back(t->dataClone());
    }

    return c;
}

QDomElement & TierCategory::toXml(QDomElement &xml) const {
    if (!root) {
        xml.setAttribute("name", name);
    }

    QDomDocument doc;

    foreach (Tier *t, subLeafs) {
        QDomElement elem = doc.createElement("tier");
        t->toXml(elem);
        xml.appendChild(elem);
    }

    foreach (TierCategory *c, subCategories) {
        QDomElement elem = doc.createElement("category");
        c->toXml(elem);
        xml.appendChild(elem);
    }

    return xml;
}

QList<Tier *> TierCategory::gatherTiers()
{
    QList<Tier*> l = subLeafs;
    foreach(TierCategory *c, subCategories) {
        l.append(c->gatherTiers());
    }
    return l;
}

void TierTree::loadFromXml(const QString &xmldata, TierMachine *boss)
{
    QDomDocument doc;
    doc.setContent(xmldata);
    QDomElement docElem = doc.documentElement();
    root.loadFromXml(docElem, boss, true);
}

QString TierTree::toXml() const
{
    QDomDocument doc;

    QDomElement main = doc.createElement("category");

    root.toXml(main);

    doc.appendChild(main);

    return doc.toString();
}

QList<Tier*> TierTree::gatherTiers()
{
    return root.gatherTiers();
}

QByteArray TierTree::buildTierList()
{
    QByteArray toWrite;

    QDataStream stream(&toWrite, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_4_5);

    root.serialize(stream, -1);

    return toWrite;
}

void TierTree::cleanCategories() {
    root.cleanCategories();
}

TierTree *TierTree::dataClone() const
{
    TierTree *t = new TierTree();

    TierCategory *rootCopy = root.dataClone();
    t->root = *rootCopy;

    /* As it was copied in the new root, data is shared, and so we have
       to break the sharing */
    rootCopy->clearWithoutDeleting();
    delete rootCopy;

    return t;
}
