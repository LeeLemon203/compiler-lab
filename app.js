document.addEventListener('DOMContentLoaded', function() {
    const navItems = document.querySelectorAll('.nav-item');
    const pages = document.querySelectorAll('.page-content');
    
    function switchPage(pageId) {
        pages.forEach(page => {
            page.classList.remove('active');
        });
        
        const targetPage = document.getElementById(`page-${pageId}`);
        if (targetPage) {
            targetPage.classList.add('active');
        }
        
        navItems.forEach(item => {
            item.classList.remove('active');
            if (item.getAttribute('data-page') === pageId) {
                item.classList.add('active');
            }
        });
    }
    
    navItems.forEach(item => {
        item.addEventListener('click', function() {
            const pageId = this.getAttribute('data-page');
            if (pageId) {
                switchPage(pageId);
            }
        });
    });
    
    const goToExp1 = document.querySelector('.go-to-exp1');
    const goToExp3 = document.querySelector('.go-to-exp3');
    
    if (goToExp1) {
        goToExp1.addEventListener('click', () => switchPage('exp1'));
    }
    if (goToExp3) {
        goToExp3.addEventListener('click', () => switchPage('exp3'));
    }
});
