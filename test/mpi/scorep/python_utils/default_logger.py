import logging


def set_default_logging_behavior(logfile=None, root_name=None):
    logger = logging.getLogger(root_name)

    logger.setLevel(logging.DEBUG)

    simple_format = logging.Formatter('%(levelname)s: %(message)s')
    detailed_format = logging.Formatter('%(asctime)s: %(levelname)s in %(name)s: %(message)s')

    if logfile is not None:
        fh = logging.FileHandler('{0}.log'.format(logfile), mode='w')
        fh.setLevel(logging.DEBUG)
        fh.setFormatter(simple_format)
        logger.addHandler(fh)

    ch = logging.StreamHandler()
    ch.setLevel(logging.INFO)
    ch.setFormatter(simple_format)
    logger.addHandler(ch)

