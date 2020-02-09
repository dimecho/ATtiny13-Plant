window.requestAnimationFrame = function() {
    return window.requestAnimationFrame || window.webkitRequestAnimationFrame || window.mozRequestAnimationFrame || window.oRequestAnimationFrame || window.msRequestAnimationFrame || function(a) {
        window.setTimeout(a, 1000 / 60)
    }
};
window.transitionEnd = function(a, c) {
    var b = !1, d = document.createElement("div");
    $(["transition", "WebkitTransition", "MozTransition", "msTransition"]).each(function(a, c) {
        if (void 0 !== d.style[c]) return b = !0, !1
    });
    b ? a.bind("webkitTransitionEnd oTransitionEnd MSTransitionEnd transitionend", function(b) {
        a.unbind("webkitTransitionEnd oTransitionEnd MSTransitionEnd transitionend");
        c(b, a)
    }) : setTimeout(function() {
        c(null, a)
    }, 0);
    return a
};
var Preloader = {
    overlay: null,
    loader: null,
    name: null,
    percentage: null,
    on_complete: null,
    total: 0,
    loaded: 0,
    image_queue: [],
    percentage_loaded: 0,
    images: [],
    show_progress: 1,
    show_percentage: 1,
    background: "#000000",
    timeout: 10,
    init: function() {
        $("img").each(function(a) {
            $(this).attr("src") && Preloader.images.push($(this).attr("src"))
        });
        Preloader.total = Preloader.images.length;
        Preloader.build();
        Preloader.load()
    },
    build: function() {
        this.overlay = $("#preloader");
        this.overlay.addClass("preloader_progress");
        this.overlay.css("background-color", this.background);
        this.progress_loader = $("<div>").addClass("preloader_loader").appendTo(this.overlay), this.progress_loader_meter = $("<div>").addClass("preloader_meter").appendTo(this.progress_loader), this.show_progress && (this.percentage = $("<div>").addClass("preloader_percentage").text(0).appendTo(this.overlay))
    },
    load: function() {
        this.percentage.data("num", 0);
        var a = "0" + (Preloader.show_percentage ? "%" : "");
        $.each(this.images, function(a, b) {
            var d = function() {
                Preloader.imageOnLoad(b)
            },
            e = new Image;
            e.src = b;
            e.complete ? d() : (e.onload = d, e.onerror = d)
        });
        setTimeout(function() {
            Preloader.overlay && Preloader.animatePercentage(Preloader.percentage_loaded,100)
        }, this.images.length ? 1000 * this.timeout : 0)
    },
    animatePercentage: function(a, c) {
        Preloader.percentage_loaded = a;
        a < c && (a++, setTimeout(function() {
            Preloader.show_progress && (b = a + (Preloader.show_percentage ? "%" : ""), Preloader.percentage.text(b)), Preloader.progress_loader_meter.width(a + "%")
            Preloader.animatePercentage(a, c)
        }, 5), 100 === a && Preloader.loadFinish())
    },
    imageOnLoad: function(a) {
        this.image_queue.push(a);
        this.image_queue.length && this.image_queue[0] === a && this.processQueue()
    },
    reQueue: function() {
        Preloader.image_queue.splice(0,1);
        Preloader.processQueue()
    },
    processQueue: function() {
        0 !== this.image_queue.length && (this.loaded++, Preloader.animatePercentage(Preloader.percentage_loaded, parseInt(this.loaded / this.total * 100, 10)), this.reQueue())
    },
    loadFinish: function() {
        transitionEnd(this.overlay, function(a, c) {
            Preloader.overlay && (Preloader.overlay.remove(), Preloader.overlay = null)
        });
        this.overlay.addClass("complete");
        $(document.body).removeClass("preloader");
        this.on_complete && this.on_complete()
    }
};
setTimeout(function() {
    $(document).ready(Preloader.init)
});